#include <stdint.h>
#include <stdbool.h>
#include <bitarray.h>

/**
 * This file handles the allocation and freeing of fixed size data cells in a memory region.
 * The allocator uses a really simple algorithm, it stores a memory region offset that is incremented
 * on allocation of a cell. The allocator does not handle fragmentation, it will only check for
 * available cells from the offset. The allocator uses a bit array to keep track of available cells.
 */

/* Data struct that handles allocation and freeing of fixed size data cells in memory region */
typedef struct fsmem {
    uint64_t avail_bitpos; /* bit position of next available cell */
    bitarray_t *avail_bitarr; /* bit array representing available data cells */
    uint64_t num_cells; /* number of cells in data region */
    uint64_t cell_size; /* number of bytes in a cell */
    uintptr_t base_addr; /* base address of data region */
} fsmem_t;

/**
 * Check if the memory region can fit count more free cells.
 *
 * @param fsmem pointer to the fsmem struct.
 * @param count number of cells to check.
 *
 * @return true indicates the data region is full, false otherwise.
 */
bool fsmem_full(fsmem_t *fsmem, uint64_t count);

/**
 * Get count many free cells in the data region.
 *
 * @param fsmem pointer to the fsmem struct.
 * @param addr pointer to base address of the resulting contiguous cell.
 * @param count number of free cells to get.
 *
 * @return -1 when data region is full, 0 on success.
 */
int fsmem_alloc(fsmem_t *fsmem, uintptr_t *addr, uint64_t count);

/**
 * Free count many available cells in the data region.
 *
 * @param fsmem pointer to the fsmem struct.
 * @param addr base address of the contiguous cell to free.
 * @param count number of cells to free.
 */
void fsmem_free(fsmem_t *fsmem, uintptr_t addr, uint64_t count);

/**
 * Initialise fixed size memory allocation struct.
 * 
 * @param fsmem pointer to the fsmem struct.
 * @param base_addr base address of the data region.
 * @param cell_size number of bytes in a cell.
 * @param num_cells number of cells in the data region.
 * @param bitarr pointer to the bitarray struct representing available data cells.
 * @param words pointer to the array of words in bitarray struct.
 * @param num_words number of words in the array of bitarray struct. This needs to be > num_cells/64. Can be calculated using roundup_bits2words64(num_cells).
 */
void fsmem_init(fsmem_t *fsmem, uintptr_t base_addr, uint64_t cell_size, uint64_t num_cells, bitarray_t *bitarr, word_t *words, word_index_t num_words);