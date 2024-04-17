#include <microkit.h>
#include <stdint.h>
#include <stdbool.h>
#include <virtio/config.h>
#include <virtio/virtq.h>
#include <virtio/mmio.h>
#include <virtio/block.h>
#include <virq.h>
#include <util.h>
#include <sddf/blk/fsmalloc.h>
#include <sddf/util/ialloc.h>
#include <sddf/blk/queue.h>

/* Uncomment this to enable debug logging */
// #define DEBUG_BLOCK

#if defined(DEBUG_BLOCK)
#define LOG_BLOCK(...) do{ printf("VIRTIO(BLOCK): "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_BLOCK(...) do{}while(0)
#endif

#define LOG_BLOCK_ERR(...) do{ printf("VIRTIO(BLOCK)|ERROR: "); printf(__VA_ARGS__); }while(0)

/* Maximum number of buffers in sddf data region */
//@ericc: as long as this number is higher than any sddf_handler->data_size we should be fine
#define SDDF_MAX_DATA_BUFFERS 4096

/* Virtio blk configuration space */
static struct virtio_blk_config virtio_blk_config;

/* Data struct that handles allocation and freeing of fixed size data cells in sDDF memory region */
static fsmalloc_t fsmalloc;
static bitarray_t fsmalloc_avail_bitarr;
static word_t fsmalloc_avail_bitarr_words[roundup_bits2words64(SDDF_MAX_DATA_BUFFERS)];

/* Bookkeeping request data between virtIO and sDDF */
typedef struct reqbk {
    uint16_t virtio_desc_head;
    uintptr_t sddf_data;
    uint16_t sddf_count;
    uint32_t sddf_block_number;
    uintptr_t virtio_data;
    uint16_t virtio_data_size;
    bool not_aligned;
} reqbk_t;
static reqbk_t reqbk[SDDF_MAX_DATA_BUFFERS];

/* Index allocator */
static ialloc_t ialloc;
static uint64_t ialloc_idxlist[SDDF_MAX_DATA_BUFFERS];

static void virtio_blk_mmio_reset(struct virtio_device *dev)
{
    dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ].ready = 0;
    dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ].last_idx = 0;
}

static int virtio_blk_mmio_get_device_features(struct virtio_device *dev, uint32_t *features)
{
    if (dev->data.Status & VIRTIO_CONFIG_S_FEATURES_OK) {
        LOG_BLOCK_ERR("driver somehow wants to read device features after FEATURES_OK\n");
    }

    switch (dev->data.DeviceFeaturesSel) {
        /* feature bits 0 to 31 */
        case 0:
            *features = BIT_LOW(VIRTIO_BLK_F_FLUSH);
            *features = *features | BIT_LOW(VIRTIO_BLK_F_BLK_SIZE);
            break;
        /* features bits 32 to 63 */
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
    /* According to virtio initialisation protocol,
       this should check what device features were set, and return the subset of features understood
       by the driver. */
    int success = 1;

    uint32_t device_features = 0;
    device_features = device_features | BIT_LOW(VIRTIO_BLK_F_FLUSH);
    device_features = device_features | BIT_LOW(VIRTIO_BLK_F_BLK_SIZE);

    switch (dev->data.DriverFeaturesSel) {
        /* feature bits 0 to 31 */
        case 0:
            success = (features == device_features);
            break;
        /* features bits 32 to 63 */
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
    uintptr_t config_base_addr = (uintptr_t)&virtio_blk_config;
    uintptr_t config_field_offset = (uintptr_t)(offset - REG_VIRTIO_MMIO_CONFIG);
    uint32_t *config_field_addr = (uint32_t *)(config_base_addr + config_field_offset);
    *ret_val = *config_field_addr;
    LOG_BLOCK("get device config with base_addr 0x%x and field_address 0x%x has value %d\n", config_base_addr, config_field_addr, *ret_val);
    return 1;
}

static int virtio_blk_mmio_set_device_config(struct virtio_device *dev, uint32_t offset, uint32_t val)
{
    uintptr_t config_base_addr = (uintptr_t)&virtio_blk_config;
    uintptr_t config_field_offset = (uintptr_t)(offset - REG_VIRTIO_MMIO_CONFIG);
    uint32_t *config_field_addr = (uint32_t *)(config_base_addr + config_field_offset);
    *config_field_addr = val;
    LOG_BLOCK("set device config with base_addr 0x%x and field_address 0x%x with value %d\n", config_base_addr, config_field_addr, val);
    return 1;
}

static void virtio_blk_used_buffer(struct virtio_device *dev, uint16_t desc)
{
    struct virtq *virtq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ].virtq;
    struct virtq_used_elem used_elem = {desc, 0};

    virtq->used->ring[virtq->used->idx % virtq->num] = used_elem;
    virtq->used->idx++;
}

static bool virtio_blk_virq_inject(struct virtio_device *dev)
{
    return virq_inject(GUEST_VCPU_ID, dev->virq);
}

static void virtio_blk_set_interrupt_status(struct virtio_device *dev, bool used_buffer, bool config_change)
{
    /* Set the reason of the irq.
       bit 0: used buffer
       bit 1: configuration change */
    dev->data.InterruptStatus = used_buffer | (config_change << 1);
}

/* Set response to virtio request to error */
static void virtio_blk_set_req_fail(struct virtio_device *dev, uint16_t desc)
{
    struct virtq *virtq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ].virtq;

    uint16_t curr_virtio_desc = desc;
    for (;virtq->desc[curr_virtio_desc].flags & VIRTQ_DESC_F_NEXT; curr_virtio_desc = virtq->desc[curr_virtio_desc].next){}
    *((uint8_t *)virtq->desc[curr_virtio_desc].addr) = VIRTIO_BLK_S_IOERR;
}

static void virtio_blk_set_req_success(struct virtio_device *dev, uint16_t desc)
{
    struct virtq *virtq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ].virtq;

    uint16_t curr_virtio_desc = desc;
    for (;virtq->desc[curr_virtio_desc].flags & VIRTQ_DESC_F_NEXT; curr_virtio_desc = virtq->desc[curr_virtio_desc].next){}
    *((uint8_t *)virtq->desc[curr_virtio_desc].addr) = VIRTIO_BLK_S_OK;
}

static bool sddf_make_req_check(blk_queue_handle_t *h, uint16_t sddf_count) {
    // Check if req store is full, if data region is full, if req queue is full
    // If these all pass then this request can be handled successfully
    if (ialloc_full(&ialloc)) {
        LOG_BLOCK_ERR("Request store is full\n");
        return false;
    }

    if (blk_req_queue_full(h)) {
        LOG_BLOCK_ERR("Request queue is full\n");
        return false;
    }

    if (fsmalloc_full(&fsmalloc, sddf_count)) {
        LOG_BLOCK_ERR("Data region is full\n");
        return false;
    }

    return true;
}

static int virtio_blk_mmio_queue_notify(struct virtio_device *dev)
{
    // @ericc: If multiqueue feature bit negotiated, should read which queue from dev->QueueNotify,
    // but for now we just assume it's the one and only default queue
    virtio_queue_handler_t *vq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ];
    struct virtq *virtq = &vq->virtq;

    blk_queue_handle_t *queue_handle = dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].queue_h;

    bool has_dropped = false; /* if any request has to be dropped due to any number of reasons, this becomes true */
    
    /* get next available request to be handled */
    uint16_t idx = vq->last_idx;

    LOG_BLOCK("------------- Driver notified device -------------\n");
    for (;idx != virtq->avail->idx; idx++) {
        uint16_t desc_head = virtq->avail->ring[idx % virtq->num];

        uint16_t curr_desc_head = desc_head;

        /* Print out what the request type is */
        struct virtio_blk_outhdr *virtio_req = (void *)virtq->desc[curr_desc_head].addr;
        LOG_BLOCK("----- Request type is 0x%x -----\n", virtio_req->type);
        
        /* Parse different requests */
        switch (virtio_req->type) {
            /* There are three parts with each block request. The header, body (which contains the data) and reply. */
            case VIRTIO_BLK_T_IN: {
                LOG_BLOCK("Request type is VIRTIO_BLK_T_IN\n");
                LOG_BLOCK("Sector (read/write offset) is %d\n", virtio_req->sector);
                
                curr_desc_head = virtq->desc[curr_desc_head].next;
                LOG_BLOCK("Descriptor index is %d, Descriptor flags are: 0x%x, length is 0x%x\n", curr_desc_head, (uint16_t)virtq->desc[curr_desc_head].flags, virtq->desc[curr_desc_head].len);

                // Converting virtio sector number to sddf block number, we are rounding down */
                uint32_t sddf_block_number = (virtio_req->sector * VIRTIO_BLK_SECTOR_SIZE) / BLK_TRANSFER_SIZE;
                // Converting bytes to the number of blocks, we are rounding up */
                uint16_t sddf_count = (virtq->desc[curr_desc_head].len + BLK_TRANSFER_SIZE - 1) / BLK_TRANSFER_SIZE;

                if (!sddf_make_req_check(queue_handle, sddf_count)) {
                    virtio_blk_set_req_fail(dev, desc_head);
                    has_dropped = true;
                    break;
                }
                
                /* Allocate data buffer from data region based on sddf_count */
                uintptr_t sddf_data;
                fsmalloc_alloc(&fsmalloc, &sddf_data, sddf_count);
                
                /* Bookkeep the virtio sddf block size translation */
                uintptr_t virtio_data = sddf_data + (virtio_req->sector * VIRTIO_BLK_SECTOR_SIZE) % BLK_TRANSFER_SIZE;
                uintptr_t virtio_data_size = virtq->desc[curr_desc_head].len;

                // Book keep the request
                reqbk_t data = {desc_head, sddf_data, sddf_count, sddf_block_number, virtio_data, virtio_data_size, false};
                uint64_t req_id;
                ialloc_alloc(&ialloc, &req_id);
                reqbk[req_id] = data;
                
                blk_enqueue_req(queue_handle, READ_BLOCKS, sddf_data, sddf_block_number, sddf_count, req_id);
                break;
            }
            case VIRTIO_BLK_T_OUT: {
                LOG_BLOCK("Request type is VIRTIO_BLK_T_OUT\n");
                LOG_BLOCK("Sector (read/write offset) is %d\n", virtio_req->sector);
                
                curr_desc_head = virtq->desc[curr_desc_head].next;
                LOG_BLOCK("Descriptor index is %d, Descriptor flags are: 0x%x, length is 0x%x\n", curr_desc_head, (uint16_t)virtq->desc[curr_desc_head].flags, virtq->desc[curr_desc_head].len);

                /* Converting virtio sector number to sddf block number, we are rounding down */
                uint32_t sddf_block_number = (virtio_req->sector * VIRTIO_BLK_SECTOR_SIZE) / BLK_TRANSFER_SIZE;
                /* Converting bytes to the number of blocks, we are rounding up */
                uint16_t sddf_count = (virtq->desc[curr_desc_head].len + BLK_TRANSFER_SIZE - 1) / BLK_TRANSFER_SIZE;
                
                bool not_aligned = ((virtio_req->sector % (BLK_TRANSFER_SIZE / VIRTIO_BLK_SECTOR_SIZE)) != 0);

                /* If the write request is not aligned to the sddf block size, we need to first read the surrounding aligned memory, overwrite that 
                   read memory on the unaligned areas we want write to, and then write the entire memory back to disk */
                if (not_aligned) {
                    if (!sddf_make_req_check(queue_handle, sddf_count)) {
                        virtio_blk_set_req_fail(dev, desc_head);
                        has_dropped = true;
                        break;
                    }

                    /* Allocate data buffer from data region based on sddf_count */
                    uintptr_t sddf_data;
                    fsmalloc_alloc(&fsmalloc, &sddf_data, sddf_count);

                    /* Bookkeep the virtio sddf block size translation */
                    uintptr_t virtio_data = sddf_data + (virtio_req->sector * VIRTIO_BLK_SECTOR_SIZE) % BLK_TRANSFER_SIZE;
                    uintptr_t virtio_data_size = virtq->desc[curr_desc_head].len;

                    // Book keep the request
                    reqbk_t data = {desc_head, sddf_data, sddf_count, sddf_block_number, virtio_data, virtio_data_size, not_aligned};
                    uint64_t req_id;
                    ialloc_alloc(&ialloc, &req_id);
                    reqbk[req_id] = data;

                    blk_enqueue_req(queue_handle, READ_BLOCKS, sddf_data, sddf_block_number, sddf_count, req_id);
                } else {
                    if (!sddf_make_req_check(queue_handle, sddf_count)) {
                        virtio_blk_set_req_fail(dev, desc_head);
                        has_dropped = true;
                        break;
                    }

                    /* Allocate data buffer from data region based on sddf_count */
                    uintptr_t sddf_data;
                    fsmalloc_alloc(&fsmalloc, &sddf_data, sddf_count);

                    /* Bookkeep the virtio sddf block size translation */
                    uintptr_t virtio_data = sddf_data + (virtio_req->sector * VIRTIO_BLK_SECTOR_SIZE) % BLK_TRANSFER_SIZE;
                    uintptr_t virtio_data_size = virtq->desc[curr_desc_head].len;

                    // Book keep the request
                    reqbk_t data = {desc_head, sddf_data, sddf_count, sddf_block_number, virtio_data, virtio_data_size, not_aligned};
                    uint64_t req_id;
                    ialloc_alloc(&ialloc, &req_id);
                    reqbk[req_id] = data;
                    
                    /* Copy data from virtio buffer to data buffer, create sddf write request and initialise it with data buffer */
                    memcpy((void *)sddf_data, (void *)virtq->desc[curr_desc_head].addr, virtq->desc[curr_desc_head].len);
                    blk_enqueue_req(queue_handle, WRITE_BLOCKS, sddf_data, sddf_block_number, sddf_count, req_id);
                }
                break;
            }
            case VIRTIO_BLK_T_FLUSH: {
                LOG_BLOCK("Request type is VIRTIO_BLK_T_FLUSH\n");

                if (!sddf_make_req_check(queue_handle, 0)) {
                    virtio_blk_set_req_fail(dev, desc_head);
                    has_dropped = true;
                    break;
                }

                // Book keep the request, @ericc: except for virtio desc, nothing else needs to be retrieved later so leave as 0
                reqbk_t data = {desc_head, 0, 0, 0, 0, 0};
                uint64_t req_id;
                ialloc_alloc(&ialloc, &req_id);
                reqbk[req_id] = data;

                blk_enqueue_req(queue_handle, FLUSH, 0, 0, 0, req_id);
                break;
            }
        }
    }

    /* Update virtq index to the next available request to be handled */
    vq->last_idx = idx;

    int success = 1;
    
    /* If any request has to be dropped due to any number of reasons, we inject an interrupt */
    if (has_dropped) {
        virtio_blk_set_interrupt_status(dev, true, false);
        success = virtio_blk_virq_inject(dev);
    }
    
    if (!blk_req_queue_plugged(queue_handle)) {
        // @ericc: there is a world where all requests to be handled during this batch are dropped and hence this notify to the other PD would be redundant
        microkit_notify(dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].ch);
    }
    
    return success;
}

int virtio_blk_handle_resp(struct virtio_device *dev) {
    blk_queue_handle_t *queue_handle = dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].queue_h;

    blk_response_status_t sddf_ret_status;
    uint16_t sddf_ret_success_count;
    uint32_t sddf_ret_id;

    bool handled = false;
    while (!blk_resp_queue_empty(queue_handle)) {
        blk_dequeue_resp(queue_handle, &sddf_ret_status, &sddf_ret_success_count, &sddf_ret_id);
        
        /* Freeing and retrieving data store */
        reqbk_t data = reqbk[sddf_ret_id];
        ialloc_free(&ialloc, sddf_ret_id);

        struct virtq *virtq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ].virtq;
        
        struct virtio_blk_outhdr *virtio_req = (void *)virtq->desc[data.virtio_desc_head].addr;
        
        uint16_t curr_virtio_desc = virtq->desc[data.virtio_desc_head].next;

        if (sddf_ret_status == SUCCESS) {
            switch (virtio_req->type) {
                case VIRTIO_BLK_T_IN: {
                    memcpy((void *)virtq->desc[curr_virtio_desc].addr, (void *)data.virtio_data, data.virtio_data_size);
                    break;
                }
                case VIRTIO_BLK_T_OUT:
                    if (data.not_aligned) {
                        /* Copy the write data into an offset into the allocated sddf data buffer */
                        memcpy((void *)data.virtio_data, (void *)virtq->desc[curr_virtio_desc].addr, data.virtio_data_size);
                        
                        reqbk_t parsed_data = {data.virtio_desc_head, data.sddf_data, data.sddf_count, data.sddf_block_number, 0, 0, false};
                        uint64_t new_sddf_id;
                        ialloc_alloc(&ialloc, &new_sddf_id);
                        reqbk[new_sddf_id] = parsed_data;

                        blk_enqueue_req(queue_handle, WRITE_BLOCKS, data.sddf_data, data.sddf_block_number, data.sddf_count, new_sddf_id);
                        microkit_notify(dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].ch);
                        continue;
                    }
                    break;
                case VIRTIO_BLK_T_FLUSH:
                    break;
            }
            virtio_blk_set_req_success(dev, data.virtio_desc_head);
        } else {
            virtio_blk_set_req_fail(dev, data.virtio_desc_head);
        }

        /* Free corresponding bookkeeping structures regardless of the request's success status */
        if (virtio_req->type == VIRTIO_BLK_T_IN || virtio_req->type == VIRTIO_BLK_T_OUT) {
            fsmalloc_free(&fsmalloc, data.sddf_data, data.sddf_count);
        }

        virtio_blk_used_buffer(dev, data.virtio_desc_head);

        handled = true;
    }

    int success = 1;

    /* We need to know if we handled any responses, if we did we inject an interrupt, if we didn't we don't inject */
    if (handled) {
        virtio_blk_set_interrupt_status(dev, true, false);
        success = virtio_blk_virq_inject(dev);
    }

    return success;
}

static void virtio_blk_config_init(struct virtio_device *dev) {
    blk_storage_info_t *config = (blk_storage_info_t *)dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].config;
    virtio_blk_config.capacity = (BLK_TRANSFER_SIZE / VIRTIO_BLK_SECTOR_SIZE) * config->size;
    virtio_blk_config.blk_size = config->sector_size;
}

static virtio_device_funs_t functions = {
    .device_reset = virtio_blk_mmio_reset,
    .get_device_features = virtio_blk_mmio_get_device_features,
    .set_driver_features = virtio_blk_mmio_set_driver_features,
    .get_device_config = virtio_blk_mmio_get_device_config,
    .set_device_config = virtio_blk_mmio_set_device_config,
    .queue_notify = virtio_blk_mmio_queue_notify,
};

void virtio_blk_init(struct virtio_device *dev,
                    struct virtio_queue_handler *vqs, size_t num_vqs,
                    size_t virq,
                    sddf_handler_t *sddf_handlers) {
    dev->data.DeviceID = DEVICE_ID_VIRTIO_BLOCK;
    dev->data.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    dev->funs = &functions;
    dev->vqs = vqs;
    dev->num_vqs = num_vqs;
    dev->virq = virq;
    dev->sddf_handlers = sddf_handlers;

    size_t sddf_data_buffers = dev->sddf_handlers->data_size / BLK_TRANSFER_SIZE;
    assert(sddf_data_buffers <= SDDF_MAX_DATA_BUFFERS);

    virtio_blk_config_init(dev);
    
    fsmalloc_init(&fsmalloc,
             dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].data,
             BLK_TRANSFER_SIZE,
             sddf_data_buffers,
             &fsmalloc_avail_bitarr,
             fsmalloc_avail_bitarr_words,
             roundup_bits2words64(sddf_data_buffers));

    ialloc_init(&ialloc, ialloc_idxlist, sddf_data_buffers);
}