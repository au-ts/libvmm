#include "virtio/config.h"
#include "virtio/virtq.h"
#include "virtio/mmio.h"
#include "virtio/block.h"
#include "util/util.h"
#include "virq.h"
#include "block/libblocksharedringbuffer/include/sddf_blk_shared_ringbuffer.h"

/* Uncomment this to enable debug logging */
#define DEBUG_BLOCK

#if defined(DEBUG_BLOCK)
#define LOG_BLOCK(...) do{ printf("VIRTIO(BLOCK): "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_BLOCK(...) do{}while(0)
#endif

#define LOG_BLOCK_ERR(...) do{ printf("VIRTIO(BLOCK)|ERROR: "); printf(__VA_ARGS__); }while(0)

// @ivanv: put in util or remove
#define BIT_LOW(n)  (1ul<<(n))
#define BIT_HIGH(n) (1ul<<(n - 32 ))

// @ericc: Maybe move this into virtio.c, and store a pointer in virtio_device struct?
static struct virtio_blk_config blk_config;

/* Mapping for command ID and its virtio descriptor */
static struct virtio_blk_cmd_store {
    uint16_t sent_cmds[SDDF_BLK_NUM_DATA_BUFFERS];
    uint32_t freelist[SDDF_BLK_NUM_DATA_BUFFERS];
    uint32_t head;
    uint32_t tail;
    uint32_t num_free;
} cmd_store;

static void virtio_blk_mmio_reset(struct virtio_device *dev)
{
    dev->vqs[VIRTIO_BLK_VIRTQ_DEFAULT].ready = 0;
    dev->vqs[VIRTIO_BLK_VIRTQ_DEFAULT].last_idx = 0;
}

static int virtio_blk_mmio_get_device_features(struct virtio_device *dev, uint32_t *features)
{
    if (dev->data.Status & VIRTIO_CONFIG_S_FEATURES_OK) {
        LOG_BLOCK_ERR("driver somehow wants to read device features after FEATURES_OK\n");
    }

    switch (dev->data.DeviceFeaturesSel) {
        // feature bits 0 to 31
        case 0:
            *features = BIT_LOW(VIRTIO_BLK_F_FLUSH);
            break;
        // features bits 32 to 63
        case 1:
            *features = BIT_HIGH(VIRTIO_F_VERSION_1);
            break;
        default:
            LOG_BLOCK_ERR("driver sets DeviceFeaturesSel to 0x%x, which doesn't make sense\n", dev->data.DeviceFeaturesSel);
            return 0;
    }
    return 1;
}

static int virtio_blk_mmio_set_driver_features(struct virtio_device *dev, uint32_t features)
{
    // According to virtio initialisation protocol,
    // this should check what device features were set, and return the subset of features understood
    // by the driver. However, for now we ignore what the driver sets, and just return the features we support.
    int success = 1;

    switch (dev->data.DriverFeaturesSel) {
        // feature bits 0 to 31
        case 0:
            success = (features & BIT_LOW(VIRTIO_BLK_F_FLUSH));
            break;
        // features bits 32 to 63
        case 1:
            success = (features == BIT_HIGH(VIRTIO_F_VERSION_1));
            break;
        default:
            LOG_BLOCK_ERR("driver sets DriverFeaturesSel to 0x%x, which doesn't make sense\n", dev->data.DriverFeaturesSel);
            success = 0;
    }

    if (success) {
        dev->data.features_happy = 1;
    }

    return success;
}

static int virtio_blk_mmio_get_device_config(struct virtio_device *dev, uint32_t offset, uint32_t *ret_val)
{
    uintptr_t config_base_addr = (uintptr_t)&blk_config;
    uintptr_t config_field_offset = (uintptr_t)(offset - REG_VIRTIO_MMIO_CONFIG);
    uint32_t *config_field_addr = (uint32_t *)(config_base_addr + config_field_offset);
    *ret_val = *config_field_addr;
    LOG_BLOCK("get device config with base_addr 0x%x and field_address 0x%x has value %d\n", config_base_addr, config_field_addr, *ret_val);
    return 1;
}

static int virtio_blk_mmio_set_device_config(struct virtio_device *dev, uint32_t offset, uint32_t val)
{
    uintptr_t config_base_addr = (uintptr_t)&blk_config;
    uintptr_t config_field_offset = (uintptr_t)(offset - REG_VIRTIO_MMIO_CONFIG);
    uint32_t *config_field_addr = (uint32_t *)(config_base_addr + config_field_offset);
    *config_field_addr = val;
    LOG_BLOCK("set device config with base_addr 0x%x and field_address 0x%x with value %d\n", config_base_addr, config_field_addr, val);
    return 1;
}

/* Set command response to error */
static void virtio_blk_command_fail(struct virtio_device *dev, uint16_t desc)
{
    struct virtq *virtq = &dev->vqs[VIRTIO_BLK_VIRTQ_DEFAULT].virtq;

    uint16_t curr_virtio_desc = desc;
    for (;virtq->desc[curr_virtio_desc].flags & VIRTQ_DESC_F_NEXT; curr_virtio_desc = virtq->desc[curr_virtio_desc].next){}
    *((uint8_t *)virtq->desc[curr_virtio_desc].addr) = VIRTIO_BLK_S_IOERR;

    struct virtq_used_elem used_elem = {desc, 0};
    uint16_t guest_idx = virtq->used->idx;

    virtq->used->ring[guest_idx % virtq->num] = used_elem;
    virtq->used->idx++;
}

static void virtio_blk_used_buffer_notify(struct virtio_device *dev)
{
    // set the reason of the irq: used buffer notification to virtio
    dev->data.InterruptStatus = BIT_LOW(0);

    bool success = virq_inject(GUEST_VCPU_ID, dev->virq);
    assert(success);
}

static int virtio_blk_store_cmd(uint16_t desc, uint32_t *id)
{
    if (cmd_store.num_free == 0) {
        return -1;
    }

    cmd_store.sent_cmds[cmd_store.head] = desc;
    *id = cmd_store.head;

    cmd_store.head = cmd_store.freelist[cmd_store.head];

    cmd_store.num_free--;
    
    return 0;
}

static uint16_t virtio_blk_retrieve_cmd(uint32_t id)
{
    assert(cmd_store.num_free < SDDF_BLK_NUM_DATA_BUFFERS);

    if (cmd_store.num_free == 0) {
        // Head points to stale index, so restore it
        cmd_store.head = id;
    }

    cmd_store.freelist[cmd_store.tail] = id;
    cmd_store.tail = id;

    cmd_store.num_free++;

    return cmd_store.sent_cmds[id];
}

static int virtio_blk_cmd_in(struct virtio_device *dev, uint16_t desc_head)
{
    virtio_queue_handler_t *vq = &dev->vqs[VIRTIO_BLK_VIRTQ_DEFAULT];
    struct virtq *virtq = &vq->virtq;
    sddf_blk_ring_handle_t *sddf_ring_handle = dev->sddf_ring_handles[SDDF_BLK_DEFAULT_RING];

    int ret = 1;

    uint16_t curr_desc_head = desc_head.next;

    uint32_t sddf_count = virtq->desc[curr_desc_head].len / SDDF_BLK_DATA_BUFFER_SIZE;
    uintptr_t addr;
    ret = sddf_blk_data_get_buffer(sddf_ring_handle, &addr, sddf_count);
    if (ret != 0) {
        LOG_BLOCK_ERR("Unable to get free buffers from sDDF blk\n");
        return 0;
    }

    /* Bookkeep this command */
    uint32_t sddf_cmd_idx;
    ret = virtio_blk_store_cmd(desc_head, &sddf_cmd_idx);
    if (ret == -1) {
        LOG_BLOCK_ERR("Unable to store virtio command \n");
        return 0;
    }
    
    ret = sddf_blk_enqueue_cmd(sddf_ring_handle, SDDF_BLK_COMMAND_READ, sddf_desc_head_idx, virtio_cmd->sector, sddf_count, sddf_cmd_idx);
    if (ret == -1) {
        LOG_BLOCK_ERR("Unable to enqueue command to sDDF blk\n");
        return 0;
    }

    return ret;
}

static int virtio_blk_mmio_queue_notify(struct virtio_device *dev)
{
    // @ericc: If multiqueue feature bit negotiated, should read which queue has been selected from dev->data->QueueSel,
    // but for now we just assume it's the one and only default queue
    virtio_queue_handler_t *vq = &dev->vqs[VIRTIO_BLK_VIRTQ_DEFAULT];
    struct virtq *virtq = &vq->virtq;

    sddf_blk_ring_handle_t *sddf_ring_handle = dev->sddf_ring_handles[SDDF_BLK_DEFAULT_RING];

    /* read the current guest index */
    uint16_t guest_idx = virtq->avail->idx;
    uint16_t idx = vq->last_idx;

    int ret = 1;

    LOG_BLOCK("------------- Driver notified device -------------\n");

    for (; idx != guest_idx; idx++) {
        uint16_t desc_head = virtq->avail->ring[idx % virtq->num];

        uint16_t curr_desc_head = desc_head;

        // Print out what the command type is
        struct virtio_blk_outhdr *virtio_cmd = (void *)virtq->desc[curr_desc_head].addr;
        LOG_BLOCK("----- Command type is 0x%x -----\n", virtio_cmd->type);
        
        // Parse different commands
        switch (virtio_cmd->type) {
            // header -> body -> reply
            case VIRTIO_BLK_T_IN: {
                LOG_BLOCK("Command type is VIRTIO_BLK_T_IN\n");
                LOG_BLOCK("Sector (read/write offset) is %d (x512)\n", virtio_cmd->sector);
                
                curr_desc_head = virtq->desc[curr_desc_head].next;
                LOG_BLOCK("Descriptor index is %d, Descriptor flags are: 0x%x, length is 0x%x\n", curr_desc_head, (uint16_t)virtq->desc[curr_desc_head].flags, virtq->desc[curr_desc_head].len);

                bool cmd_split = false;

                uint32_t sddf_count = virtq->desc[curr_desc_head].len / SDDF_BLK_DATA_BUFFER_SIZE;
                
                if (sddf_blk_data_full(sddf_ring_handle, sddf_count)) {
                    LOG_BLOCK_ERR("sDDF blk data buffer is full\n");
                    ret = 0;
                }

                if (sddf_blk_data_end(sddf_ring_handle, sddf_count)) {
                    LOG_BLOCK("sDDF blk data buffer will overflow, splitting command into two\n");
                    ret = virtio_
                } else {
                    ret = virtio_blk_cmd_in(dev, desc_head);
                }

                if (ret != 1) {
                    virtio_blk_command_fail(dev, desc_head);
                }
                break;
            }
            case VIRTIO_BLK_T_OUT: {
                LOG_BLOCK("Command type is VIRTIO_BLK_T_OUT\n");
                LOG_BLOCK("Sector (read/write offset) is %d (x512)\n", virtio_cmd->sector);
                curr_desc_head = virtq->desc[curr_desc_head].next;
                LOG_BLOCK("Descriptor index is %d, Descriptor flags are: 0x%x, length is 0x%x\n", curr_desc_head, (uint16_t)virtq->desc[curr_desc_head].flags, virtq->desc[curr_desc_head].len);
                uintptr_t data = virtq->desc[curr_desc_head].addr;
                
                uint32_t sddf_count = virtq->desc[curr_desc_head].len / SDDF_BLK_DATA_BUFFER_SIZE;
                uint32_t sddf_desc_head_idx;
                ret = sddf_blk_get_desc(sddf_ring_handle, &sddf_desc_head_idx, sddf_count);
                if (ret == -1) {
                    LOG_BLOCK_ERR("Unable to get descriptor from sDDF blk\n");
                    virtio_blk_command_fail(dev, desc_head);
                    return 0;
                }
                
                uint32_t sddf_curr_desc_idx = sddf_desc_head_idx;
                // Copy a sector of data from vring into sddf buffer
                for (int i = 0; i < sddf_count; i++) {
                    memcpy((void *)sddf_desc_handle->descs[sddf_curr_desc_idx].addr, (void *)(data + (i * SDDF_BLK_DATA_BUFFER_SIZE)), SDDF_BLK_DATA_BUFFER_SIZE);
                    sddf_curr_desc_idx = sddf_desc_handle->descs[sddf_curr_desc_idx].next;
                }

                /* Bookkeep this command */
                uint32_t sddf_cmd_idx;
                ret = virtio_blk_store_cmd(desc_head, &sddf_cmd_idx);
                if (ret == -1) {
                    LOG_BLOCK_ERR("Unable to store virtio command \n");
                    virtio_blk_command_fail(dev, desc_head);
                    return 0;
                }
                
                ret = sddf_blk_enqueue_cmd(sddf_ring_handle, SDDF_BLK_COMMAND_WRITE, sddf_desc_head_idx, virtio_cmd->sector, sddf_count, sddf_cmd_idx);
                if (ret == -1) {
                    LOG_BLOCK_ERR("Unable to enqueue command to sDDF blk\n");
                    virtio_blk_command_fail(dev, desc_head);
                    return 0;
                }
                break;
            }
            case VIRTIO_BLK_T_FLUSH: {
                LOG_BLOCK("Command type is VIRTIO_BLK_T_FLUSH\n");
                /* Bookkeep this command */
                uint32_t sddf_cmd_idx;
                ret = virtio_blk_store_cmd(desc_head, &sddf_cmd_idx);
                if (ret == -1) {
                    LOG_BLOCK_ERR("Unable to store virtio command \n");
                    virtio_blk_command_fail(dev, desc_head);
                    return 0;
                }

                ret = sddf_blk_enqueue_cmd(sddf_ring_handle, SDDF_BLK_COMMAND_FLUSH, 0, 0, 0, sddf_cmd_idx);
                if (ret == -1) {
                    LOG_BLOCK_ERR("Unable to enqueue command to sDDF blk\n");
                    virtio_blk_command_fail(dev, desc_head);
                    return 0;
                }
                break;
            }
        }
    }

    vq->last_idx = idx;

    if (ret == 1) {
        /* Notify other side our command has been sent */
        microkit_notify(dev->sddf_ch);
    } else {
        /* Notify vm guest their command failed */
        virtio_blk_used_buffer_notify(dev);
    }
    
    return 1;
}

void virtio_blk_handle_resp(struct virtio_device *dev) {
    sddf_blk_ring_handle_t *sddf_ring_handle = dev->sddf_ring_handles[SDDF_BLK_DEFAULT_RING];
    sddf_blk_desc_handle_t *sddf_desc_handle = sddf_ring_handle->desc_handle;

    sddf_blk_response_status_t sddf_ret_status;
    uint32_t sddf_ret_desc;
    uint16_t sddf_ret_count;
    uint32_t sddf_ret_id;
    while (sddf_blk_dequeue_resp(sddf_ring_handle, &sddf_ret_status, &sddf_ret_desc, &sddf_ret_count, &sddf_ret_id) != -1) {
        uint16_t virtio_desc = virtio_blk_retrieve_cmd(sddf_ret_id, &virtio_desc);
        struct virtq *virtq = &dev->vqs[VIRTIO_BLK_VIRTQ_DEFAULT].virtq;
        
        /* Error case, respond to virtio with error */
        if (sddf_ret_status == SDDF_BLK_RESPONSE_ERROR) {
            virtio_blk_command_fail(dev, virtio_desc);
            continue;
        }
        
        struct virtio_blk_outhdr *virtio_cmd = (void *)virtq->desc[virtio_desc].addr;
        uint16_t curr_virtio_desc = virtq->desc[virtio_desc].next;
        switch (virtio_cmd->type) {
            case VIRTIO_BLK_T_IN: {
                uintptr_t data = virtq->desc[curr_virtio_desc].addr;

                uint32_t sddf_curr_desc_idx = sddf_ret_desc;
                // Copy a sector from sddf buffer into data
                for (int i = 0; i < sddf_ret_count; i++) {
                    memcpy((void * )(data + (i * SDDF_BLK_DATA_BUFFER_SIZE)), (void *)(sddf_desc_handle->descs[sddf_curr_desc_idx].addr), SDDF_BLK_DATA_BUFFER_SIZE);
                    sddf_curr_desc_idx = sddf_desc_handle->descs[sddf_curr_desc_idx].next;   
                }
                
                sddf_blk_free_desc(dev->sddf_ring_handles[SDDF_BLK_DEFAULT_RING], sddf_ret_desc);

                curr_virtio_desc = virtq->desc[curr_virtio_desc].next;
                *((uint8_t *)virtq->desc[curr_virtio_desc].addr) = VIRTIO_BLK_S_OK;
                break;
            }
            case VIRTIO_BLK_T_OUT: {
                sddf_blk_free_desc(dev->sddf_ring_handles[SDDF_BLK_DEFAULT_RING], sddf_ret_desc);
                
                curr_virtio_desc = virtq->desc[curr_virtio_desc].next;
                *((uint8_t *)virtq->desc[curr_virtio_desc].addr) = VIRTIO_BLK_S_OK;
                break;
            }
            case VIRTIO_BLK_T_FLUSH: {
                curr_virtio_desc = virtq->desc[curr_virtio_desc].next;
                *((uint8_t *)virtq->desc[curr_virtio_desc].addr) = VIRTIO_BLK_S_OK;
                break;
            }
        }

        struct virtq_used_elem used_elem = {virtio_desc, 0};
        uint16_t guest_idx = virtq->used->idx;

        virtq->used->ring[guest_idx % virtq->num] = used_elem;
        virtq->used->idx++;
    }

    // set the reason of the irq: used buffer notification to virtio
    dev->data.InterruptStatus = BIT_LOW(0);

    bool success = virq_inject(GUEST_VCPU_ID, dev->virq);
    assert(success);
}

static virtio_device_funs_t functions = {
    .device_reset = virtio_blk_mmio_reset,
    .get_device_features = virtio_blk_mmio_get_device_features,
    .set_driver_features = virtio_blk_mmio_set_driver_features,
    .get_device_config = virtio_blk_mmio_get_device_config,
    .set_device_config = virtio_blk_mmio_set_device_config,
    .queue_notify = virtio_blk_mmio_queue_notify,
};

static void virtio_blk_config_init() 
{
    blk_config.capacity = VIRTIO_BLK_CAPACITY;
}

static void virtio_blk_cmd_store_init()
{
    cmd_store.head = 0;
    cmd_store.tail = SDDF_BLK_NUM_DATA_BUFFERS - 1;
    cmd_store.num_free = SDDF_BLK_NUM_DATA_BUFFERS;
    for (int i = 0; i < SDDF_BLK_NUM_DATA_BUFFERS - 1; i++) {
        cmd_store.freelist[i] = i + 1;
    }
    cmd_store.freelist[SDDF_BLK_NUM_DATA_BUFFERS - 1] = -1;
}

void virtio_blk_init(struct virtio_device *dev,
                    struct virtio_queue_handler *vqs, size_t num_vqs,
                    size_t virq,
                    void **sddf_ring_handles, size_t sddf_ch) {
    dev->data.DeviceID = DEVICE_ID_VIRTIO_BLOCK;
    dev->data.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    dev->funs = &functions;
    dev->vqs = vqs;
    dev->num_vqs = num_vqs;
    dev->virq = virq;
    dev->sddf_ring_handles = sddf_ring_handles;
    dev->sddf_ch = sddf_ch;
    
    virtio_blk_config_init();
    virtio_blk_cmd_store_init();
}