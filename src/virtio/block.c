#include "virtio/config.h"
#include "virtio/virtq.h"
#include "virtio/mmio.h"
#include "virtio/block.h"
#include "util/util.h"
#include "virq.h"
#include <sddf/blk/shared_queue.h>

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
static struct virtio_blk_config virtio_blk_config;

/* Mapping for request ID and its virtio descriptor */
static struct virtio_blk_req_store {
    uint16_t sent_reqs[SDDF_BLK_MAX_DATA_BUFFERS]; /* index is request ID, maps to virtio descriptor head */
    uint32_t freelist[SDDF_BLK_MAX_DATA_BUFFERS]; /* index is free request ID, maps to next free request ID */
    uint32_t head;
    uint32_t tail;
    uint32_t num_free;
} req_store;

static void virtio_blk_mmio_reset(struct virtio_device *dev)
{
    // Poll ((blk_storage_info_t *)dev->config)->ready until it is ready
    while (!((blk_storage_info_t *)dev->config)->ready) {
        LOG_BLOCK("waiting for device to be ready\n");
    }

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
            *features = BIT_LOW(VIRTIO_BLK_F_FLUSH) | BIT_LOW(VIRTIO_BLK_F_BLK_SIZE);
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
            success = (features == (BIT_LOW(VIRTIO_BLK_F_FLUSH) | BIT_LOW(VIRTIO_BLK_F_BLK_SIZE)));
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

/**
 * Check if the request store is full.
 * 
 * @return true if request store is full, false otherwise.
 */
static inline bool virtio_blk_req_store_full()
{
    return req_store.num_free == 0;
}

/**
 * Allocate a request store slot for a new virtio request.
 * 
 * @param desc virtio descriptor to store in a request store slot 
 * @param id pointer to request ID allocated
 * @return 0 on success, -1 on failure
 */
static inline int virtio_blk_req_store_allocate(uint16_t desc, uint32_t *id)
{
    if (virtio_blk_req_store_full()) {
        return -1;
    }

    // Store descriptor into head of request store
    req_store.sent_reqs[req_store.head] = desc;
    *id = req_store.head;

    // Update head to next free request store slot
    req_store.head = req_store.freelist[req_store.head];

    // Update number of free request store slots
    req_store.num_free--;
    
    return 0;
}

/**
 * Retrieve and free a request store slot.
 * 
 * @param id request ID to be retrieved
 * @return virtio descriptor stored in slot
 */
static inline uint16_t virtio_blk_req_store_retrieve(uint32_t id)
{
    assert(req_store.num_free < SDDF_BLK_MAX_DATA_BUFFERS);

    if (req_store.num_free == 0) {
        // Head points to stale index, so restore it
        req_store.head = id;
    }

    req_store.freelist[req_store.tail] = id;
    req_store.tail = id;

    req_store.num_free++;

    return req_store.sent_reqs[id];
}

/**
 * Convert a bit position to the address of the corresponding data buffer.
 * 
 * @param bitpos bit position of the data buffer
 * @return address of the data buffer
 */
static inline uintptr_t blk_data_region_bitpos_to_addr(struct virtio_device *dev, uint32_t bitpos)
{
    return ((blk_data_region_t *)dev->data_region_handlers[SDDF_BLK_DEFAULT_HANDLE])->addr + ((uintptr_t)bitpos * ((blk_storage_info_t *)dev->config)->blocksize);
}

/**
 * Convert an address to the bit position of the corresponding data buffer.
 * 
 * @param addr address of the data buffer
 * @return bit position of the data buffer
 */
static inline uint32_t blk_data_region_addr_to_bitpos(struct virtio_device *dev, uintptr_t addr)
{
    return (uint32_t)((addr - ((blk_data_region_t *)dev->data_region_handlers[SDDF_BLK_DEFAULT_HANDLE])->addr) / ((blk_storage_info_t *)dev->config)->blocksize);
}

/**
 * Check if count number of buffers will overflow the end of the data region.
 * 
 * @param count number of buffers to check
 * @return true if count number of buffers will overflow the end of the data region, false otherwise
 */
static inline bool blk_data_region_overflow(struct virtio_device *dev, uint16_t count)
{
    return (((blk_data_region_t *)dev->data_region_handlers[SDDF_BLK_DEFAULT_HANDLE])->avail_bitpos + count > ((blk_data_region_t *)dev->data_region_handlers[SDDF_BLK_DEFAULT_HANDLE])->num_buffers);
}

// TAGGED
/**
 * Check if the data region is full; it has count number of free buffers available.
 *
 * @param count number of buffers to check.
 *
 * @return true indicates the data region is full, false otherwise.
 */
static bool blk_data_region_full(struct virtio_device *dev, uint16_t count)
{
    blk_data_region_t *blk_data_region = dev->data_region_handlers[SDDF_BLK_DEFAULT_HANDLE];

    if (count > blk_data_region->num_buffers) {
        return true;
    }
    
    unsigned int start_bitpos = blk_data_region->avail_bitpos;
    if (blk_data_region_overflow(dev, count)) {
        start_bitpos = 0;
    }

    // Create a bit mask with count many 1's
    bitarray_t bitarr_mask;
    word_t words[roundup_bits2words64(count)];
    bitarray_init(&bitarr_mask, words, roundup_bits2words64(count));
    bitarray_set_region(&bitarr_mask, 0, count);

    if (bitarray_cmp_region(blk_data_region->avail_bitarr, start_bitpos, &bitarr_mask, 0, count)) {
        return false;
    }

    return true;
}

/**
 * Get count many free buffers in the data region.
 *
 * @param addr pointer to base address of the resulting contiguous buffer.
 * @param count number of free buffers to get.
 *
 * @return -1 when data region is full, 0 on success.
 */
static int blk_data_region_get_buffer(struct virtio_device *dev, uintptr_t *addr, uint16_t count)
{
    blk_data_region_t *blk_data_region = dev->data_region_handlers[SDDF_BLK_DEFAULT_HANDLE];

    if (blk_data_region_full(dev, count)) {
        return -1;
    }

    if (blk_data_region_overflow(dev, count)) {
        blk_data_region->avail_bitpos = 0;
    }

    *addr = blk_data_region_bitpos_to_addr(dev, blk_data_region->avail_bitpos);
    
    // Set the next count many bits as unavailable
    bitarray_clear_region(blk_data_region->avail_bitarr, blk_data_region->avail_bitpos, count);

    // Update the bitpos
    uint32_t new_bitpos = blk_data_region->avail_bitpos + count;
    if (new_bitpos == blk_data_region->num_buffers) {
        new_bitpos = 0;
    }
    blk_data_region->avail_bitpos = new_bitpos;

    return 0;
}

/**
 * Free count many available buffers in the data region.
 *
 * @param addr base address of the contiguous buffer to free.
 * @param count number of buffers to free.
 */
static void blk_data_region_free_buffer(struct virtio_device *dev, uintptr_t addr, uint16_t count)
{   
    blk_data_region_t *blk_data_region = dev->data_region_handlers[SDDF_BLK_DEFAULT_HANDLE];

    unsigned int start_bitpos = blk_data_region_addr_to_bitpos(dev, addr);

    // Assert here in case we try to free buffers that overflow the data region
    assert(start_bitpos + count <= blk_data_region->num_buffers);

    // Set the next count many bits as available
    bitarray_set_region(blk_data_region->avail_bitarr, start_bitpos, count);
}

static int virtio_blk_mmio_queue_notify(struct virtio_device *dev)
{
    // @ericc: If multiqueue feature bit negotiated, should read which queue from dev->QueueNotify,
    // but for now we just assume it's the one and only default queue
    virtio_queue_handler_t *vq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ];
    struct virtq *virtq = &vq->virtq;

    blk_queue_handle_t *queue_handle = dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE];

    bool has_error = false; /* if any request has to be dropped due to any number of reasons (req queue full, req_store full), this becomes true */
    
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

                uint16_t sddf_count = virtq->desc[curr_desc_head].len / ((blk_storage_info_t *)dev->config)->blocksize;
                
                // Check if req store is full, if data region is full, if req queue is full
                // If these all pass then this request can be handled successfully
                if (virtio_blk_req_store_full()) {
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

                if (blk_data_region_full(dev, sddf_count)) {
                    LOG_BLOCK_ERR("Data region is full\n");
                    virtio_blk_set_req_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                // Book keep the request
                uint32_t req_id;
                virtio_blk_req_store_allocate(desc_head, &req_id);
                
                // Allocate data buffer from data region based on sddf_count
                // Pass this allocated data buffer to sddf read request, then enqueue it
                uintptr_t sddf_data;
                blk_data_region_get_buffer(dev, &sddf_data, sddf_count);
                blk_enqueue_req(queue_handle, READ_BLOCKS, sddf_data, virtio_req->sector, sddf_count, req_id);
                break;
            }
            case VIRTIO_BLK_T_OUT: {
                LOG_BLOCK("Request type is VIRTIO_BLK_T_OUT\n");
                LOG_BLOCK("Sector (read/write offset) is %d\n", virtio_req->sector);
                
                curr_desc_head = virtq->desc[curr_desc_head].next;
                LOG_BLOCK("Descriptor index is %d, Descriptor flags are: 0x%x, length is 0x%x\n", curr_desc_head, (uint16_t)virtq->desc[curr_desc_head].flags, virtq->desc[curr_desc_head].len);
                
                uintptr_t virtio_data = virtq->desc[curr_desc_head].addr;
                uint16_t sddf_count = virtq->desc[curr_desc_head].len / ((blk_storage_info_t *)dev->config)->blocksize;
                
                // Check if req store is full, if data region is full, if req queue is full
                // If these all pass then this request can be handled successfully
                if (virtio_blk_req_store_full()) {
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

                if (blk_data_region_full(dev, sddf_count)) {
                    LOG_BLOCK_ERR("Data region is full\n");
                    virtio_blk_set_req_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                // Book keep the request
                uint32_t req_id;
                virtio_blk_req_store_allocate(desc_head, &req_id);
                
                // Allocate data buffer from data region based on sddf_count
                // Copy data from virtio buffer to data buffer, create sddf write request and initialise it with data buffer, then enqueue it
                uintptr_t sddf_data;
                blk_data_region_get_buffer(dev, &sddf_data, sddf_count);
                memcpy((void *)sddf_data, (void *)virtio_data, sddf_count * ((blk_storage_info_t *)dev->config)->blocksize);
                blk_enqueue_req(queue_handle, WRITE_BLOCKS, sddf_data, virtio_req->sector, sddf_count, req_id);
                break;
            }
            case VIRTIO_BLK_T_FLUSH: {
                LOG_BLOCK("Request type is VIRTIO_BLK_T_FLUSH\n");

                // Check if req store is full, if req queue is full
                // If these all pass then this request can be handled successfully
                // If fail, then set response to virtio request to error
                if (virtio_blk_req_store_full()) {
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

                // Book keep the request
                uint32_t req_id;
                virtio_blk_req_store_allocate(desc_head, &req_id);

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
        microkit_notify(dev->sddf_ch[SDDF_BLK_DEFAULT_CH_INDEX]);
    }
    
    return 1;
}

void virtio_blk_handle_resp(struct virtio_device *dev) {
    blk_queue_handle_t *queue_handle = dev->sddf_handlers[SDDF_BLK_DEFAULT_HANDLE];

    blk_response_status_t sddf_ret_status;
    uintptr_t sddf_ret_addr;
    uint16_t sddf_ret_count;
    uint16_t sddf_ret_success_count;
    uint32_t sddf_ret_id;
    while (!blk_resp_queue_empty(queue_handle)) {
        blk_dequeue_resp(queue_handle, &sddf_ret_status, &sddf_ret_addr, &sddf_ret_count, &sddf_ret_success_count, &sddf_ret_id);
        
        /* Freeing and retrieving request store */
        uint16_t virtio_desc = virtio_blk_req_store_retrieve(sddf_ret_id);
        struct virtq *virtq = &dev->vqs[VIRTIO_BLK_DEFAULT_VIRTQ].virtq;
        struct virtio_blk_outhdr *virtio_req = (void *)virtq->desc[virtio_desc].addr;

        /* Responding error to virtio if needed */
        if (sddf_ret_status == SEEK_ERROR) {
            virtio_blk_set_req_fail(dev, virtio_desc);
        } else {
            uint16_t curr_virtio_desc = virtq->desc[virtio_desc].next;
            switch (virtio_req->type) {
                case VIRTIO_BLK_T_IN: {
                    // Copy successful counts from the data buffer to the virtio buffer
                    memcpy((void *)virtq->desc[curr_virtio_desc].addr, (void *)sddf_ret_addr, sddf_ret_success_count * ((blk_storage_info_t *)dev->config)->blocksize);
                    // Free the data buffer
                    blk_data_region_free_buffer(dev, sddf_ret_addr, sddf_ret_count);
                    curr_virtio_desc = virtq->desc[curr_virtio_desc].next;
                    *((uint8_t *)virtq->desc[curr_virtio_desc].addr) = VIRTIO_BLK_S_OK;
                    break;
                }
                case VIRTIO_BLK_T_OUT: {
                    // Free the data buffer
                    blk_data_region_free_buffer(dev, sddf_ret_addr, sddf_ret_count);
                    curr_virtio_desc = virtq->desc[curr_virtio_desc].next;
                    *((uint8_t *)virtq->desc[curr_virtio_desc].addr) = VIRTIO_BLK_S_OK;
                    break;
                }
                case VIRTIO_BLK_T_FLUSH: {
                    *((uint8_t *)virtq->desc[curr_virtio_desc].addr) = VIRTIO_BLK_S_OK;
                    break;
                }
            }
        }
        
        virtio_blk_used_buffer(dev, virtio_desc);
    }

    virtio_blk_used_buffer_virq_inject(dev);
}

void virtio_blk_handle_config_change(struct virtio_device *dev) {

}

static virtio_device_funs_t functions = {
    .device_reset = virtio_blk_mmio_reset,
    .get_device_features = virtio_blk_mmio_get_device_features,
    .set_driver_features = virtio_blk_mmio_set_driver_features,
    .get_device_config = virtio_blk_mmio_get_device_config,
    .set_device_config = virtio_blk_mmio_set_device_config,
    .queue_notify = virtio_blk_mmio_queue_notify,
};

static void virtio_blk_config_init(struct virtio_device *dev) 
{
    blk_storage_info_t *config = (blk_storage_info_t *)dev->config;
    virtio_blk_config.blk_size = config->blocksize;
    virtio_blk_config.capacity = (config->blocksize / 512) * config->size;
}

static void virtio_blk_req_store_init(struct virtio_device *dev, unsigned int num_buffers)
{
    req_store.head = 0;
    req_store.tail = num_buffers - 1;
    req_store.num_free = num_buffers;
    for (unsigned int i = 0; i < num_buffers - 1; i++) {
        req_store.freelist[i] = i + 1;
    }
    req_store.freelist[num_buffers - 1] = -1;
}

void virtio_blk_init(struct virtio_device *dev,
                    struct virtio_queue_handler *vqs, size_t num_vqs,
                    size_t virq,
                    void *config,
                    void **data_region_handlers,
                    void **sddf_handlers, size_t *sddf_ch) {
    dev->data.DeviceID = DEVICE_ID_VIRTIO_BLOCK;
    dev->data.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    dev->funs = &functions;
    dev->vqs = vqs;
    dev->num_vqs = num_vqs;
    dev->virq = virq;
    dev->config = config;
    dev->data_region_handlers = data_region_handlers;
    dev->sddf_handlers = sddf_handlers;
    dev->sddf_ch = sddf_ch;
    
    virtio_blk_config_init(dev);
    virtio_blk_req_store_init(dev, SDDF_BLK_MAX_DATA_BUFFERS);
}