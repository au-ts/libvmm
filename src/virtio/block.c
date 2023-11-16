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

static struct virtio_blk_config blk_config;

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

static int virtio_blk_mmio_queue_notify(struct virtio_device *dev)
{
    virtio_queue_handler_t *vq = &dev->vqs[VIRTIO_BLK_VIRTQ_DEFAULT];
    struct virtq *virtq = &vq->virtq;

    sddf_blk_ring_handle_t *sddf_ring_handle = dev->sddf_ring_handles[SDDF_BLK_DEFAULT_RING];
    sddf_blk_desc_handle_t *sddf_desc_handle = sddf_ring_handle->desc_handle;

    /* read the current guest index */
    uint16_t guest_idx = virtq->avail->idx;
    uint16_t idx = vq->last_idx;

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
                // uintptr_t data = virtq->desc[curr_desc_head].addr;
                // LOG_BLOCK("Data is %s\n", (char *)data);
                break;
            }
            case VIRTIO_BLK_T_OUT: {
                LOG_BLOCK("Command type is VIRTIO_BLK_T_OUT\n");
                LOG_BLOCK("Sector (read/write offset) is %d (x512)\n", virtio_cmd->sector);
                curr_desc_head = virtq->desc[curr_desc_head].next;
                LOG_BLOCK("Descriptor index is %d, Descriptor flags are: 0x%x, length is 0x%x\n", curr_desc_head, (uint16_t)virtq->desc[curr_desc_head].flags, virtq->desc[curr_desc_head].len);
                uintptr_t data = virtq->desc[curr_desc_head].addr;
                
                uint32_t sddf_count = virtq->desc[curr_desc_head].len / VIRTIO_BLK_SECTOR_SIZE;
                uint32_t sddf_desc_head_idx;
                
                sddf_blk_get_desc(sddf_ring_handle, &sddf_desc_head_idx, sddf_count);
                uint32_t sddf_curr_desc_idx = sddf_desc_head_idx;

                // Copy a sector from data into sddf buffer
                for (int i = 0; i < sddf_count; i++) {
                    memcpy((void *)sddf_desc_handle->descs[sddf_curr_desc_idx].addr, (void *)(data + (i * VIRTIO_BLK_SECTOR_SIZE)), VIRTIO_BLK_SECTOR_SIZE);
                    sddf_curr_desc_idx = sddf_desc_handle->descs[sddf_curr_desc_idx].next;
                }
                
                sddf_blk_enqueue_cmd(sddf_ring_handle, SDDF_BLK_COMMAND_WRITE, sddf_desc_head_idx, virtio_cmd->sector, sddf_count, 0);

                // uint32_t sddf_desc_head_idx;
                // uint32_t count = 3;
                // LOG_BLOCK("Get sDDF descriptors\n");
                // int success = sddf_blk_get_desc(sddf_ring_handle, &sddf_desc_head_idx, count);
                // if (success != 0) {
                //     LOG_BLOCK_ERR("Failed to get descriptor from sddf data region\n");
                //     return 0;
                // }
                // uint32_t curr_desc_idx = sddf_desc_head_idx;
                // sddf_blk_desc_t *curr_desc = &sddf_desc_handle->descs[sddf_desc_head_idx];
                // for (int i = 0; i<count; i++) {
                //     LOG_BLOCK("sDDF descriptor index: %d, hasNext: %d, next: %d\n", curr_desc_idx, curr_desc->has_next, curr_desc->next);
                //     *(char *)curr_desc->addr = 'P';
                //     curr_desc_idx = curr_desc->next;
                //     curr_desc = &sddf_desc_handle->descs[curr_desc->next];
                // }

                // success = sddf_blk_enqueue_cmd(sddf_ring_handle, SDDF_BLK_COMMAND_WRITE, sddf_desc_head_idx, 0, count, 0);
                // if (success != 0) {
                //     LOG_BLOCK_ERR("Failed to enqueue command to sddf command region\n");
                //     return 0;
                // }
                // LOG_BLOCK("sDDF command enqueued\n");

                // sddf_blk_command_code_t ret_code;
                // uint32_t ret_desc;
                // uint32_t ret_sector;
                // uint16_t ret_count;
                // uint32_t ret_id;
                // success = sddf_blk_dequeue_cmd(sddf_ring_handle, &ret_code, &ret_desc, &ret_sector, &ret_count, &ret_id);
                // LOG_BLOCK("sDDF command dequeued\n");
                // LOG_BLOCK("sDDF command code: %d, sector: %d, count: %d, id: %d\n", ret_code, ret_sector, ret_count, ret_id);
                // curr_desc_idx = ret_desc;
                // curr_desc = &sddf_desc_handle->descs[ret_desc];
                // for (int i = 0; i<count; i++) {
                //     LOG_BLOCK("sDDF descriptor index: %d, hasNext: %d, next: %d\n", curr_desc_idx, curr_desc->has_next, curr_desc->next);
                //     LOG_BLOCK("sDDF descriptor data: %c\n", *(char *)curr_desc->addr);
                //     curr_desc_idx = curr_desc->next;
                //     curr_desc = &sddf_desc_handle->descs[curr_desc->next];
                // }

                // success = sddf_blk_enqueue_resp(sddf_ring_handle, SDDF_BLK_RESPONSE_OK, ret_desc, ret_count, 0);
                // if (success != 0) {
                //     LOG_BLOCK_ERR("Failed to enqueue response to sddf response region\n");
                //     return 0;
                // }
                // LOG_BLOCK("sDDF OK response enqueued\n");

                // sddf_blk_response_status_t ret_resp_status;
                // uint32_t ret_resp_desc;
                // uint16_t ret_resp_count;
                // uint32_t ret_resp_id;
                // success = sddf_blk_dequeue_resp(sddf_ring_handle, &ret_resp_status, &ret_resp_desc, &ret_resp_count, &ret_resp_id);
                // if (success != 0) {
                //     LOG_BLOCK_ERR("Failed to dequeue response from sddf response region\n");
                //     return 0;
                // }
                // LOG_BLOCK("sDDF response dequeued\n");
                // LOG_BLOCK("sDDF response status: %d, descriptor index head: %d, count: %d, id: %d\n", ret_resp_status, ret_resp_desc, ret_resp_count, ret_resp_id);
                
                // LOG_BLOCK("sDDF okay, now freeing descriptors, we have freelist head: %d, tail: %d\n", sddf_ring_handle->freelist_handle->head, sddf_ring_handle->freelist_handle->tail);
                // sddf_blk_free_desc(sddf_ring_handle, ret_desc);
                // LOG_BLOCK("sDDF descriptors freed, we have freelist head: %d, tail: %d\n", sddf_ring_handle->freelist_handle->head, sddf_ring_handle->freelist_handle->tail);
                break;
            }
            case VIRTIO_BLK_T_FLUSH: {
                LOG_BLOCK("Command type is VIRTIO_BLK_T_FLUSH\n");
                break;
            }
        }

        // For descriptor chains with more than one descriptor, the last descriptor has the VIRTQ_DESC_F_NEXT flag set to 0
        // to indicate that it is the last descriptor in the chain. That descriptor does not seem to serve any other purpose.
        // This loop brings us to the last descriptor in the chain.
        while (virtq->desc[curr_desc_head].flags & VIRTQ_DESC_F_NEXT) {
            // LOG_BLOCK("Descriptor index is %d, Descriptor flags are: 0x%x, length is 0x%x\n", curr_desc_head, (uint16_t)virtq->desc[curr_desc_head].flags, virtq->desc[curr_desc_head].len);
            curr_desc_head = virtq->desc[curr_desc_head].next;
        }
        // LOG_BLOCK("Descriptor index is %d, Descriptor flags are: 0x%x, length is 0x%x\n", curr_desc_head, (uint16_t)virtq->desc[curr_desc_head].flags, virtq->desc[curr_desc_head].len);

        // Respond OK for this command to the driver
        // by writing VIRTIO_BLK_S_OK to the final descriptor address
        *((uint8_t *)virtq->desc[curr_desc_head].addr) = VIRTIO_BLK_S_OK;
        

        // set the reason of the irq
        dev->data.InterruptStatus = BIT_LOW(0);

        struct virtq_used_elem used_elem = {desc_head, 0};
        uint16_t guest_idx = virtq->used->idx;

        virtq->used->ring[guest_idx % virtq->num] = used_elem;
        virtq->used->idx++;

        bool success = virq_inject(GUEST_VCPU_ID, dev->virq);
        assert(success);
    }

    vq->last_idx = idx;
    
    return 1;
}

void virtio_blk_handle_resp(struct virtio_device *dev) {

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
}