#include "virtio/config.h"
#include "virtio/virtq.h"
#include "virtio/mmio.h"
#include "virtio/block.h"
#include "virq.h"
#include <util/util.h>
#include <util/fsmem.h>
#include <util/datastore.h>
#include <sddf/blk/shared_queue.h>

/* Uncomment this to enable debug logging */
// #define DEBUG_BLOCK

#if defined(DEBUG_BLOCK)
#define LOG_BLOCK(...) do{ printf("VIRTIO(BLOCK): "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_BLOCK(...) do{}while(0)
#endif

#define LOG_BLOCK_ERR(...) do{ printf("VIRTIO(BLOCK)|ERROR: "); printf(__VA_ARGS__); }while(0)

static struct virtio_blk_config virtio_blk_config;

/* Data struct that handles allocation and freeing of fixed size data cells in sDDF memory region */
static fsmem_t fsmem_data;
static bitarray_t fsmem_avail_bitarr;
static word_t fsmem_avail_bitarr_words[roundup_bits2words64(SDDF_BLK_MAX_DATA_BUFFERS)];

/* Handle storing and retrieving of request data between virtIO and sDDF */
typedef struct req_data {
    uint16_t virtio_desc_head;
    uintptr_t sddf_data;
    uint16_t sddf_count;
} req_data_t;
static datastore_t reqstore;
static req_data_t reqstore_data[SDDF_BLK_MAX_DATA_BUFFERS];
static uint64_t reqstore_nextfree[SDDF_BLK_MAX_DATA_BUFFERS];
static bool reqstore_used[SDDF_BLK_MAX_DATA_BUFFERS];

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
            success = (features == (BIT_LOW(VIRTIO_BLK_F_FLUSH)));
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

static void virtio_blk_used_buffer_virq_inject(struct virtio_device *dev)
{
    // set the reason of the irq: used buffer notification to virtio
    dev->data.InterruptStatus = BIT_LOW(0);

    bool success = virq_inject(GUEST_VCPU_ID, dev->virq);
    assert(success);
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

static int virtio_blk_mmio_queue_notify(struct virtio_device *dev)
{
    // @ericc: If multiqueue feature bit negotiated, should read which queue from dev->QueueNotify,
    // but for now we just assume it's the one and only default queue
    virtio_queue_handler_t *vq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ];
    struct virtq *virtq = &vq->virtq;

    blk_queue_handle_t *queue_handle = dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].queue_h;

    bool has_error = false; /* if any request has to be dropped due to any number of reasons, this becomes true */
    
    /* get next available request to be handled */
    uint16_t idx = vq->last_idx;

    LOG_BLOCK("------------- Driver notified device -------------\n");
    for (;idx != virtq->avail->idx; idx++) {
        uint16_t desc_head = virtq->avail->ring[idx % virtq->num];

        uint16_t curr_desc_head = desc_head;

        // Print out what the request type is
        struct virtio_blk_outhdr *virtio_req = (void *)virtq->desc[curr_desc_head].addr;
        LOG_BLOCK("----- Request type is 0x%x -----\n", virtio_req->type);
        
        // Parse different requests
        switch (virtio_req->type) {
            // header -> body -> reply
            case VIRTIO_BLK_T_IN: {
                LOG_BLOCK("Request type is VIRTIO_BLK_T_IN\n");
                LOG_BLOCK("Sector (read/write offset) is %d\n", virtio_req->sector);
                
                curr_desc_head = virtq->desc[curr_desc_head].next;
                LOG_BLOCK("Descriptor index is %d, Descriptor flags are: 0x%x, length is 0x%x\n", curr_desc_head, (uint16_t)virtq->desc[curr_desc_head].flags, virtq->desc[curr_desc_head].len);

                // Since we are converting bytes to the number of blocks, we need to round up
                // uint16_t sddf_count = (virtq->desc[curr_desc_head].len + ((blk_storage_info_t *)dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].config)->blocksize - 1) / ((blk_storage_info_t *)dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].config)->blocksize;
                // uint32_t sddf_block_number = virtio_req->sector / (((blk_storage_info_t *)dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].config)->blocksize / VIRTIO_BLK_SECTOR_SIZE);
                uint32_t sddf_block_number = virtio_req->sector;
                uint16_t sddf_count = virtq->desc[curr_desc_head].len / 512;

                // Check if req store is full, if data region is full, if req queue is full
                // If these all pass then this request can be handled successfully
                if (datastore_full(&reqstore)) {
                    LOG_BLOCK_ERR("Request store is full\n");
                    virtio_blk_set_req_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                if (blk_req_queue_full(queue_handle)) {
                    LOG_BLOCK_ERR("Request queue is full\n");
                    virtio_blk_set_req_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                if (fsmem_full(&fsmem_data, sddf_count)) {
                    LOG_BLOCK_ERR("Data region is full\n");
                    virtio_blk_set_req_fail(dev, desc_head);
                    has_error = true;
                    break;
                }
                
                // Allocate data buffer from data region based on sddf_count
                uintptr_t sddf_data;
                fsmem_alloc(&fsmem_data, &sddf_data, sddf_count);

                // Book keep the request
                req_data_t req_data = {desc_head, sddf_data, sddf_count};
                uint64_t req_id;
                datastore_alloc(&reqstore, &req_data, &req_id);
                
                // Pass this allocated data buffer to sddf read request, then enqueue it
                blk_enqueue_req(queue_handle, READ_BLOCKS, sddf_data, sddf_block_number, sddf_count, req_id);
                break;
            }
            case VIRTIO_BLK_T_OUT: {
                LOG_BLOCK("Request type is VIRTIO_BLK_T_OUT\n");
                LOG_BLOCK("Sector (read/write offset) is %d\n", virtio_req->sector);
                
                curr_desc_head = virtq->desc[curr_desc_head].next;
                LOG_BLOCK("Descriptor index is %d, Descriptor flags are: 0x%x, length is 0x%x\n", curr_desc_head, (uint16_t)virtq->desc[curr_desc_head].flags, virtq->desc[curr_desc_head].len);
                
                uintptr_t virtio_data = virtq->desc[curr_desc_head].addr;
                // Since we are converting bytes to the number of blocks, we need to round up
                // uint16_t sddf_count = (virtq->desc[curr_desc_head].len + ((blk_storage_info_t *)dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].config)->blocksize - 1) / ((blk_storage_info_t *)dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].config)->blocksize;
                // uint32_t sddf_block_number = virtio_req->sector / (((blk_storage_info_t *)dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].config)->blocksize / VIRTIO_BLK_SECTOR_SIZE);
                uint32_t sddf_block_number = virtio_req->sector;
                uint16_t sddf_count = virtq->desc[curr_desc_head].len / 512;

                // Check if req store is full, if data region is full, if req queue is full
                // If these all pass then this request can be handled successfully
                if (datastore_full(&reqstore)) {
                    LOG_BLOCK_ERR("Request store is full\n");
                    virtio_blk_set_req_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                if (blk_req_queue_full(queue_handle)) {
                    LOG_BLOCK_ERR("Request queue is full\n");
                    virtio_blk_set_req_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                if (fsmem_full(&fsmem_data, sddf_count)) {
                    LOG_BLOCK_ERR("Data region is full\n");
                    virtio_blk_set_req_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                // Allocate data buffer from data region based on sddf_count
                uintptr_t sddf_data;
                fsmem_alloc(&fsmem_data, &sddf_data, sddf_count);

                // Book keep the request
                req_data_t req_data = {desc_head, sddf_data, sddf_count};
                uint64_t req_id;
                datastore_alloc(&reqstore, &req_data, &req_id);
                
                // Copy data from virtio buffer to data buffer, create sddf write request and initialise it with data buffer
                memcpy((void *)sddf_data, (void *)virtio_data, virtq->desc[curr_desc_head].len);
                blk_enqueue_req(queue_handle, WRITE_BLOCKS, sddf_data, sddf_block_number, sddf_count, req_id);
                break;
            }
            case VIRTIO_BLK_T_FLUSH: {
                LOG_BLOCK("Request type is VIRTIO_BLK_T_FLUSH\n");

                // Check if req store is full, if req queue is full
                // If these all pass then this request can be handled successfully
                // If fail, then set response to virtio request to error
                if (datastore_full(&reqstore)) {
                    LOG_BLOCK_ERR("Request store is full\n");
                    virtio_blk_set_req_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                if (blk_req_queue_full(queue_handle)) {
                    LOG_BLOCK_ERR("Request queue is full\n");
                    virtio_blk_set_req_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                // Book keep the request, here sddf_data and sddf_count is not used so leave as 0
                req_data_t req_data = {desc_head, 0, 0};
                uint64_t req_id;
                datastore_alloc(&reqstore, &req_data, &req_id);

                // Create sddf flush request and enqueue it
                blk_enqueue_req(queue_handle, FLUSH, 0, 0, 0, req_id);
                break;
            }
        }
    }

    // Update virtq index to the next available request to be handled
    vq->last_idx = idx;
    
    if (has_error) {
        virtio_blk_used_buffer_virq_inject(dev);
    }
    
    if (!blk_req_queue_plugged(queue_handle)) {
        // @ericc: there is a world where all requests to be handled during this batch are dropped and hence this notify to the other PD would be redundant
        microkit_notify(dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].ch);
    }
    
    return 1;
}

void virtio_blk_handle_resp(struct virtio_device *dev) {
    blk_queue_handle_t *queue_handle = dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].queue_h;

    blk_response_status_t sddf_ret_status;
    uint16_t sddf_ret_success_count;
    uint32_t sddf_ret_id;

    // @ericc: we need to know if we handled any responses so we can inject an interrupt
    bool handled = false;
    while (!blk_resp_queue_empty(queue_handle)) {
        handled = true;
        blk_dequeue_resp(queue_handle, &sddf_ret_status, &sddf_ret_success_count, &sddf_ret_id);
        
        /* Freeing and retrieving data store */
        req_data_t req_data;
        int ret = datastore_retrieve(&reqstore, sddf_ret_id, &req_data);
        assert(ret == 0);
        struct virtq *virtq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ].virtq;
        struct virtio_blk_outhdr *virtio_req = (void *)virtq->desc[req_data.virtio_desc_head].addr;
        
        // Free corresponding bookkeeping structures regardless of the request's success status
        switch (virtio_req->type) {
            case VIRTIO_BLK_T_IN:
            case VIRTIO_BLK_T_OUT: {
                // Free the data buffer
                fsmem_free(&fsmem_data, req_data.sddf_data, req_data.sddf_count);
                break;
            }
            case VIRTIO_BLK_T_FLUSH:
                break;
        }
        
        if (sddf_ret_status == SUCCESS) {
            uint16_t curr_virtio_desc = virtq->desc[req_data.virtio_desc_head].next;
            switch (virtio_req->type) {
                case VIRTIO_BLK_T_IN: {
                    // Copy successful counts from the data buffer to the virtio buffer
                    memcpy((void *)virtq->desc[curr_virtio_desc].addr, (void *)req_data.sddf_data, virtq->desc[curr_virtio_desc].len);
                    break;
                }
                case VIRTIO_BLK_T_OUT:
                    break;
                case VIRTIO_BLK_T_FLUSH:
                    break;
            }
            virtio_blk_set_req_success(dev, req_data.virtio_desc_head);
        } else {
            virtio_blk_set_req_fail(dev, req_data.virtio_desc_head);
        }
        
        virtio_blk_used_buffer(dev, req_data.virtio_desc_head);
    }

    if (handled) {
        virtio_blk_used_buffer_virq_inject(dev);
    }
}

static void virtio_blk_config_init(struct virtio_device *dev) {
    blk_storage_info_t *config = (blk_storage_info_t *)dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].config;
    virtio_blk_config.capacity = (config->blocksize / VIRTIO_BLK_SECTOR_SIZE) * config->size; // Number of 512-byte sectors
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

    virtio_blk_config_init(dev);

    fsmem_init(&fsmem_data,
             dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE].data,
             VIRTIO_BLK_SECTOR_SIZE, // @ericc: change when we have blocksize translation layer
             SDDF_BLK_MAX_DATA_BUFFERS,
             &fsmem_avail_bitarr,
             fsmem_avail_bitarr_words,
             roundup_bits2words64(SDDF_BLK_MAX_DATA_BUFFERS));

    datastore_init(&reqstore, reqstore_data, sizeof(req_data_t), reqstore_nextfree, reqstore_used, SDDF_BLK_MAX_DATA_BUFFERS);
}