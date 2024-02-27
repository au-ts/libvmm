#include <stdint.h>
#include <bitarray.h>
#include "smalloc.h"

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