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

// TAGGED
// @ericc: maybe put this into util.c?
/* Returns uint32_t where all bits above and including position is set to 1 */
#define MASK_ABOVE_POSITION_INCLUSIVE(position) (~(((uint32_t)1 << (position)) - 1))
/* Returns uint32_t where all bits below position is set to 1 */
#define MASK_BELOW_POSITION_EXCLUSIVE(position) (((uint32_t)1 << (position)) - 1)

// @ericc: Maybe move this into virtio.c, and store a pointer in virtio_device struct?
static struct virtio_blk_config blk_config;

// @ericc: Put this into virtio_device struct?
/* Mapping for command ID and its virtio descriptor */
static struct virtio_blk_cmd_store {
    uint16_t sent_cmds[SDDF_BLK_NUM_DATA_BUFFERS]; /* index is command ID, maps to virtio descriptor head */
    uint32_t freelist[SDDF_BLK_NUM_DATA_BUFFERS]; /* index is free command ID, maps to next free command ID */
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

static void virtio_blk_used_buffer(struct virtio_device *dev, uint16_t desc)
{
    struct virtq *virtq = &dev->vqs[VIRTIO_BLK_VIRTQ_DEFAULT].virtq;
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

/* Set response to virtio command to error */
static void virtio_blk_set_cmd_fail(struct virtio_device *dev, uint16_t desc)
{
    struct virtq *virtq = &dev->vqs[VIRTIO_BLK_VIRTQ_DEFAULT].virtq;

    uint16_t curr_virtio_desc = desc;
    for (;virtq->desc[curr_virtio_desc].flags & VIRTQ_DESC_F_NEXT; curr_virtio_desc = virtq->desc[curr_virtio_desc].next){}
    *((uint8_t *)virtq->desc[curr_virtio_desc].addr) = VIRTIO_BLK_S_IOERR;
}

/**
 * Check if the command store is full.
 * 
 * @return true if command store is full, false otherwise.
 */
static inline bool virtio_blk_cmd_store_full()
{
    return cmd_store.num_free == 0;
}

/**
 * Allocate a command store slot for a new virtio command.
 * 
 * @param desc virtio descriptor to store in a command store slot 
 * @param id pointer to command ID allocated
 * @return 0 on success, -1 on failure
 */
static inline int virtio_blk_cmd_store_allocate(uint16_t desc, uint32_t *id)
{
    if (virtio_blk_cmd_store_full()) {
        return -1;
    }

    // Store descriptor into head of command store
    cmd_store.sent_cmds[cmd_store.head] = desc;
    *id = cmd_store.head;

    // Update head to next free command store slot
    cmd_store.head = cmd_store.freelist[cmd_store.head];

    // Update number of free command store slots
    cmd_store.num_free--;
    
    return 0;
}

/**
 * Retrieve and free a command store slot.
 * 
 * @param id command ID to be retrieved
 * @return virtio descriptor stored in slot
 */
static inline uint16_t virtio_blk_cmd_store_retrieve(uint32_t id)
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

/**
 * Convert a bit position to the address of the corresponding data buffer.
 * 
 * @param bitpos bit position of the data buffer
 * @return address of the data buffer
 */
static inline uintptr_t blk_data_region_bitpos_to_addr(struct virtio_device *dev, uint32_t bitpos)
{
    return ((blk_data_region_t *)dev->data_region_handles[SDDF_BLK_DEFAULT_RING])->addr + ((uintptr_t)bitpos * SDDF_BLK_DATA_BUFFER_SIZE);
}

/**
 * Convert an address to the bit position of the corresponding data buffer.
 * 
 * @param addr address of the data buffer
 * @return bit position of the data buffer
 */
static inline uint32_t blk_data_region_addr_to_bitpos(struct virtio_device *dev, uintptr_t addr)
{
    return (uint32_t)((addr - ((blk_data_region_t *)dev->data_region_handles[SDDF_BLK_DEFAULT_RING])->addr) / SDDF_BLK_DATA_BUFFER_SIZE);
}

/**
 * Check if count number of buffers will overflow the end of the data region.
 * 
 * @param count number of buffers to check
 * @return true if count number of buffers will overflow the end of the data region, false otherwise
 */
static inline bool blk_data_region_overflow(struct virtio_device *dev, uint16_t count)
{
    return (((blk_data_region_t *)dev->data_region_handles[SDDF_BLK_DEFAULT_RING])->avail_bitpos + count > ((blk_data_region_t *)dev->data_region_handles[SDDF_BLK_DEFAULT_RING])->num_buffers);
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
    blk_data_region_t *blk_data_region = dev->data_region_handles[SDDF_BLK_DEFAULT_RING];

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
    blk_data_region_t *blk_data_region = dev->data_region_handles[SDDF_BLK_DEFAULT_RING];

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
    blk_data_region_t *blk_data_region = dev->data_region_handles[SDDF_BLK_DEFAULT_RING];

    unsigned int start_bitpos = blk_data_region_addr_to_bitpos(dev, addr);

    // Assert here in case we try to free buffers that overflow the data region
    assert(start_bitpos + count <= blk_data_region->num_buffers);

    // Set the next count many bits as available
    bitarray_set_region(blk_data_region->avail_bitarr, start_bitpos, count);
}

static int virtio_blk_mmio_queue_notify(struct virtio_device *dev)
{
    // @ericc: If multiqueue feature bit negotiated, should read which queue has been selected from dev->data->QueueSel,
    // but for now we just assume it's the one and only default queue
    virtio_queue_handler_t *vq = &dev->vqs[VIRTIO_BLK_VIRTQ_DEFAULT];
    struct virtq *virtq = &vq->virtq;

    sddf_blk_ring_handle_t *sddf_ring_handle = dev->sddf_ring_handles[SDDF_BLK_DEFAULT_RING];

    bool has_error = false; /* if any command has to be dropped due to any number of reasons (ring buffer full, cmd_store full), this becomes true */
    
    /* get next available command to be handled */
    uint16_t idx = vq->last_idx;

    LOG_BLOCK("------------- Driver notified device -------------\n");
    for (;idx != virtq->avail->idx; idx++) {
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

                uint16_t sddf_count = virtq->desc[curr_desc_head].len / SDDF_BLK_DATA_BUFFER_SIZE;
                
                // Check if cmd store is full, if data region is full, if cmd ring is full
                // If these all pass then this command can be handled successfully
                if (virtio_blk_cmd_store_full()) {
                    LOG_BLOCK_ERR("Command store is full\n");
                    virtio_blk_set_cmd_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                if (sddf_blk_cmd_ring_full(sddf_ring_handle)) {
                    LOG_BLOCK_ERR("Command ring is full\n");
                    virtio_blk_set_cmd_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                if (blk_data_region_full(dev, sddf_count)) {
                    LOG_BLOCK_ERR("Data region is full\n");
                    virtio_blk_set_cmd_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                // Book keep the command
                uint32_t cmd_id;
                virtio_blk_cmd_store_allocate(desc_head, &cmd_id);
                
                // Allocate data buffer from data region based on sddf_count
                // Pass this allocated data buffer to sddf read command, then enqueue it
                uintptr_t sddf_data;
                blk_data_region_get_buffer(dev, &sddf_data, sddf_count);
                sddf_blk_enqueue_cmd(sddf_ring_handle, SDDF_BLK_COMMAND_READ, sddf_data, virtio_cmd->sector, sddf_count, cmd_id);
                break;
            }
            case VIRTIO_BLK_T_OUT: {
                LOG_BLOCK("Command type is VIRTIO_BLK_T_OUT\n");
                LOG_BLOCK("Sector (read/write offset) is %d (x512)\n", virtio_cmd->sector);
                
                curr_desc_head = virtq->desc[curr_desc_head].next;
                LOG_BLOCK("Descriptor index is %d, Descriptor flags are: 0x%x, length is 0x%x\n", curr_desc_head, (uint16_t)virtq->desc[curr_desc_head].flags, virtq->desc[curr_desc_head].len);
                
                uintptr_t virtio_data = virtq->desc[curr_desc_head].addr;
                uint16_t sddf_count = virtq->desc[curr_desc_head].len / SDDF_BLK_DATA_BUFFER_SIZE;
                
                // Check if cmd store is full, if data region is full, if cmd ring is full
                // If these all pass then this command can be handled successfully
                if (virtio_blk_cmd_store_full()) {
                    LOG_BLOCK_ERR("Command store is full\n");
                    virtio_blk_set_cmd_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                if (sddf_blk_cmd_ring_full(sddf_ring_handle)) {
                    LOG_BLOCK_ERR("Command ring is full\n");
                    virtio_blk_set_cmd_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                if (blk_data_region_full(dev, sddf_count)) {
                    LOG_BLOCK_ERR("Data region is full\n");
                    virtio_blk_set_cmd_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                // Book keep the command
                uint32_t cmd_id;
                virtio_blk_cmd_store_allocate(desc_head, &cmd_id);
                
                // Allocate data buffer from data region based on sddf_count
                // Copy data from virtio buffer to data buffer, create sddf write command and initialise it with data buffer, then enqueue it
                uintptr_t sddf_data;
                blk_data_region_get_buffer(dev, &sddf_data, sddf_count);
                memcpy((void *)sddf_data, (void *)virtio_data, sddf_count * SDDF_BLK_DATA_BUFFER_SIZE);
                sddf_blk_enqueue_cmd(sddf_ring_handle, SDDF_BLK_COMMAND_WRITE, sddf_data, virtio_cmd->sector, sddf_count, cmd_id);
                break;
            }
            case VIRTIO_BLK_T_FLUSH: {
                LOG_BLOCK("Command type is VIRTIO_BLK_T_FLUSH\n");

                // Check if cmd store is full, if cmd ring is full
                // If these all pass then this command can be handled successfully
                // If fail, then set response to virtio command to error
                if (virtio_blk_cmd_store_full()) {
                    LOG_BLOCK_ERR("Command store is full\n");
                    virtio_blk_set_cmd_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                if (sddf_blk_cmd_ring_full(sddf_ring_handle)) {
                    LOG_BLOCK_ERR("Command ring is full\n");
                    virtio_blk_set_cmd_fail(dev, desc_head);
                    has_error = true;
                    break;
                }

                // Book keep the command
                uint32_t cmd_id;
                virtio_blk_cmd_store_allocate(desc_head, &cmd_id);

                // Create sddf flush command and enqueue it
                sddf_blk_enqueue_cmd(sddf_ring_handle, SDDF_BLK_COMMAND_FLUSH, 0, 0, 0, cmd_id);
                break;
            }
        }
    }

    // Update virtq index to the next available command to be handled
    vq->last_idx = idx;
    
    if (has_error) {
        virtio_blk_used_buffer_virq_inject(dev);
    }
    
    if (!sddf_blk_cmd_ring_plugged(sddf_ring_handle)) {
        // @ericc: there is a world where all commands to be handled during this batch are dropped and hence this notify to the other PD would be redundant
        microkit_notify(dev->sddf_ch);
    }
    
    return 1;
}

void virtio_blk_handle_resp(struct virtio_device *dev) {
    sddf_blk_ring_handle_t *sddf_ring_handle = dev->sddf_ring_handles[SDDF_BLK_DEFAULT_RING];

    sddf_blk_response_status_t sddf_ret_status;
    uintptr_t sddf_ret_addr;
    uint16_t sddf_ret_count;
    uint16_t sddf_ret_success_count;
    uint32_t sddf_ret_id;
    while (!sddf_blk_resp_ring_empty(sddf_ring_handle)) {
        sddf_blk_dequeue_resp(sddf_ring_handle, &sddf_ret_status, &sddf_ret_addr, &sddf_ret_count, &sddf_ret_success_count, &sddf_ret_id);
        
        /* Freeing and retrieving command store */
        uint16_t virtio_desc = virtio_blk_cmd_store_retrieve(sddf_ret_id);
        struct virtq *virtq = &dev->vqs[VIRTIO_BLK_VIRTQ_DEFAULT].virtq;
        struct virtio_blk_outhdr *virtio_cmd = (void *)virtq->desc[virtio_desc].addr;

        /* Responding error to virtio if needed */
        if (sddf_ret_status == SDDF_BLK_RESPONSE_ERROR) {
            virtio_blk_set_cmd_fail(dev, virtio_desc);
        } else {
            uint16_t curr_virtio_desc = virtq->desc[virtio_desc].next;
            switch (virtio_cmd->type) {
                case VIRTIO_BLK_T_IN: {
                    // Copy successful counts from the data buffer to the virtio buffer
                    memcpy((void *)virtq->desc[curr_virtio_desc].addr, (void *)sddf_ret_addr, sddf_ret_success_count * SDDF_BLK_DATA_BUFFER_SIZE);
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

static virtio_device_funs_t functions = {
    .device_reset = virtio_blk_mmio_reset,
    .get_device_features = virtio_blk_mmio_get_device_features,
    .set_driver_features = virtio_blk_mmio_set_driver_features,
    .get_device_config = virtio_blk_mmio_get_device_config,
    .set_device_config = virtio_blk_mmio_set_device_config,
    .queue_notify = virtio_blk_mmio_queue_notify,
};

// @ericc: should these be hardcoded? can initialise via a configuration file
static void virtio_blk_config_init() 
{
    blk_config.capacity = VIRTIO_BLK_CAPACITY;
}

static void virtio_blk_cmd_store_init(unsigned int num_buffers)
{
    cmd_store.head = 0;
    cmd_store.tail = num_buffers - 1;
    cmd_store.num_free = num_buffers;
    for (unsigned int i = 0; i < num_buffers - 1; i++) {
        cmd_store.freelist[i] = i + 1;
    }
    cmd_store.freelist[num_buffers - 1] = -1;
}

void virtio_blk_init(struct virtio_device *dev,
                    struct virtio_queue_handler *vqs, size_t num_vqs,
                    size_t virq,
                    void **data_region_handles,
                    void **sddf_ring_handles, size_t sddf_ch) {
    dev->data.DeviceID = DEVICE_ID_VIRTIO_BLOCK;
    dev->data.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    dev->funs = &functions;
    dev->vqs = vqs;
    dev->num_vqs = num_vqs;
    dev->virq = virq;
    dev->data_region_handles = data_region_handles;
    dev->sddf_ring_handles = sddf_ring_handles;
    dev->sddf_ch = sddf_ch;
    
    virtio_blk_config_init();
    virtio_blk_cmd_store_init(SDDF_BLK_NUM_DATA_BUFFERS);

    print_bitarray(((blk_data_region_t *)dev->data_region_handles[SDDF_BLK_DEFAULT_RING])->avail_bitarr);
}