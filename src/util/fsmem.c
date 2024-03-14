#include <stdint.h>
#include <bitarray.h>
#include "fsmem.h"

/**
 * Convert a bit position to the address of the corresponding data cell.
 * 
 * @param bitpos bit position of the data cell
 * @return address of the data cell
 */
static inline uintptr_t bitpos_to_addr(fsmem_t *fsmem, uint64_t bitpos)
{
    return fsmem->base_addr + (uintptr_t)(bitpos * fsmem->cell_size);
}

/**
 * Convert an address to the bit position of the corresponding data cell.
 * 
 * @param addr address of the data cell
 * @return bit position of the data cell
 */
static inline uint64_t addr_to_bitpos(fsmem_t *fsmem, uintptr_t addr)
{
    return (uint64_t)(addr - fsmem->base_addr) / fsmem->cell_size;
}

/**
 * Check if count number of cells will overflow the end of the data region.
 * 
 * @param count number of cells to check
 * @return true if count number of cells will overflow the end of the data region, false otherwise
 */
static inline bool fsmem_overflow(fsmem_t *fsmem, uint64_t count)
{
    return (fsmem->avail_bitpos + count > fsmem->num_cells);
}

bool fsmem_full(fsmem_t *fsmem, uint64_t count)
{
    if (count > fsmem->num_cells) {
        return true;
    }
    
    unsigned int start_bitpos = fsmem->avail_bitpos;
    if (fsmem_overflow(fsmem, count)) {
        start_bitpos = 0;
    }

    // Create a bit mask with count many 1's
    bitarray_t bitarr_mask;
    word_t words[roundup_bits2words64(count)];
    bitarray_init(&bitarr_mask, words, roundup_bits2words64(count));
    bitarray_set_region(&bitarr_mask, 0, count);

    if (bitarray_cmp_region(fsmem->avail_bitarr, start_bitpos, &bitarr_mask, 0, count)) {
        return false;
    }

    return true;
}

void fsmem_free(fsmem_t *fsmem, uintptr_t addr, uint64_t count)
{   
    unsigned int start_bitpos = addr_to_bitpos(fsmem, addr);

    // Assert here in case we try to free cells that overflow the data region
    // assert(start_bitpos + count <= fsmem->num_cells);

    // Set the next count many bits as available
    bitarray_set_region(fsmem->avail_bitarr, start_bitpos, count);
}

int fsmem_alloc(fsmem_t *fsmem, uintptr_t *addr, uint64_t count)
{
    if (fsmem_full(fsmem, count)) {
        return -1;
    }

    if (fsmem_overflow(fsmem, count)) {
        fsmem->avail_bitpos = 0;
    }

    *addr = bitpos_to_addr(fsmem, fsmem->avail_bitpos);
    
    // Set the next count many bits as unavailable
    bitarray_clear_region(fsmem->avail_bitarr, fsmem->avail_bitpos, count);

    // Update the bitpos
    uint64_t new_bitpos = fsmem->avail_bitpos + count;
    if (new_bitpos == fsmem->num_cells) {
        new_bitpos = 0;
    }
    fsmem->avail_bitpos = new_bitpos;

    return 0;
}

void fsmem_init(fsmem_t *fsmem, uintptr_t base_addr, uint64_t cell_size, uint64_t num_cells, bitarray_t *bitarr, word_t *words, word_index_t num_words) {
    bitarray_init(bitarr, words, num_words);
    
    fsmem->avail_bitpos = 0;
    fsmem->avail_bitarr = bitarr;
    fsmem->base_addr = base_addr;
    fsmem->cell_size = cell_size;
    fsmem->num_cells = num_cells;
    
    /* Set all available bits to 1 to indicate all cells are available */ 
    bitarray_set_region(bitarr, 0, num_cells);
}