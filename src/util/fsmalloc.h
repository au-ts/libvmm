#include <stdint.h>
#include <stdbool.h>
#include <bitarray.h>

/* Data struct that handles allocation and freeing of data buffers in sDDF shared memory region */
typedef struct smalloc {
    uint32_t avail_bitpos; /* bit position of next avail buffer */
    bitarray_t *avail_bitarr; /* bit array representing avail data buffers */
    uint32_t num_buffers; /* number of buffers in data region */
    uintptr_t addr; /* encoded base address of data region */
} smalloc_t;

/**
 * Check if the memory region can fit count more free buffers.
 *
 * @param count number of buffers to check.
 *
 * @return true indicates the data region is full, false otherwise.
 */
bool blk_data_region_full(smalloc_t sm, uint16_t count);

/**
 * Get count many free buffers in the data region.
 *
 * @param addr pointer to base address of the resulting contiguous buffer.
 * @param count number of free buffers to get.
 *
 * @return -1 when data region is full, 0 on success.
 */
int blk_data_region_get_buffer(uintptr_t *addr, uint16_t count);

/**
 * Free count many available buffers in the data region.
 *
 * @param addr base address of the contiguous buffer to free.
 * @param count number of buffers to free.
 */
void blk_data_region_free_buffer(uintptr_t addr, uint16_t count);