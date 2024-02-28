#include <stdint.h>
#include <bitarray.h>
#include "fsm.h"

/**
 * Convert a bit position to the address of the corresponding data cell.
 * 
 * @param bitpos bit position of the data cell
 * @return address of the data cell
 */
static inline uintptr_t bitpos_to_addr(fsm_t *fsm, uint64_t bitpos)
{
    return fsm->base_addr + (uintptr_t)(bitpos * fsm->cell_size);
}

/**
 * Convert an address to the bit position of the corresponding data cell.
 * 
 * @param addr address of the data cell
 * @return bit position of the data cell
 */
static inline uint64_t addr_to_bitpos(fsm_t *fsm, uintptr_t addr)
{
    return (uint64_t)(addr - fsm->base_addr) / fsm->cell_size;
}

/**
 * Check if count number of cells will overflow the end of the data region.
 * 
 * @param count number of cells to check
 * @return true if count number of cells will overflow the end of the data region, false otherwise
 */
static inline bool fsm_overflow(fsm_t *fsm, uint64_t count)
{
    return (fsm->avail_bitpos + count > fsm->num_cells);
}

bool fsm_full(fsm_t *fsm, uint64_t count)
{
    if (count > fsm->num_cells) {
        return true;
    }
    
    unsigned int start_bitpos = fsm->avail_bitpos;
    if (fsm_overflow(fsm, count)) {
        start_bitpos = 0;
    }

    // Create a bit mask with count many 1's
    bitarray_t bitarr_mask;
    word_t words[roundup_bits2words64(count)];
    bitarray_init(&bitarr_mask, words, roundup_bits2words64(count));
    bitarray_set_region(&bitarr_mask, 0, count);

    if (bitarray_cmp_region(fsm->avail_bitarr, start_bitpos, &bitarr_mask, 0, count)) {
        return false;
    }

    return true;
}

void fsm_free(fsm_t *fsm, uintptr_t addr, uint64_t count)
{   
    unsigned int start_bitpos = addr_to_bitpos(fsm, addr);

    // Assert here in case we try to free cells that overflow the data region
    // assert(start_bitpos + count <= fsm->num_cells);

    // Set the next count many bits as available
    bitarray_set_region(fsm->avail_bitarr, start_bitpos, count);
}

int fsm_alloc(fsm_t *fsm, uintptr_t *addr, uint64_t count)
{
    if (fsm_full(fsm, count)) {
        return -1;
    }

    if (fsm_overflow(fsm, count)) {
        fsm->avail_bitpos = 0;
    }

    *addr = bitpos_to_addr(fsm, fsm->avail_bitpos);
    
    // Set the next count many bits as unavailable
    bitarray_clear_region(fsm->avail_bitarr, fsm->avail_bitpos, count);

    // Update the bitpos
    uint64_t new_bitpos = fsm->avail_bitpos + count;
    if (new_bitpos == fsm->num_cells) {
        new_bitpos = 0;
    }
    fsm->avail_bitpos = new_bitpos;

    return 0;
}

void fsm_init(fsm_t *fsm, uintptr_t base_addr, uint64_t cell_size, uint64_t num_cells, bitarray_t *bitarr, word_t *words, word_index_t num_words) {
    bitarray_init(bitarr, words, num_words);
    
    fsm->avail_bitpos = 0;
    fsm->avail_bitarr = bitarr;
    fsm->base_addr = base_addr;
    fsm->cell_size = cell_size;
    fsm->num_cells = num_cells;
    
    /* Set all available bits to 1 to indicate all cells are available */ 
    bitarray_set_region(bitarr, 0, num_cells);
}