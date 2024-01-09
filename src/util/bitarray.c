#include "bitarray.h"

#define SET_REGION(arr,start,len)    _set_region((arr),(start),(len),FILL_REGION)
#define CLEAR_REGION(arr,start,len)  _set_region((arr),(start),(len),ZERO_REGION)
#define TOGGLE_REGION(arr,start,len) _set_region((arr),(start),(len),SWAP_REGION)

#define WORD_MAX  (~(word_t)0)

// need to check for length == 0, undefined behaviour if uint64_t >> 64 etc
#define bitmask(nbits,type) ((nbits) ? ~(type)0 >> (sizeof(type)*8-(nbits)): (type)0)
#define bitmask32(nbits) bitmask(nbits,uint32_t)
#define bitmask64(nbits) bitmask(nbits,uint64_t)

#define bitset64_wrd(pos) ((pos) >> 6)
#define bitset64_idx(pos) ((pos) & 63)

void bitarray_init(bitarray_t *bitarr, word_t *words, word_addr_t num_of_words)
{
    bitarr->words = words;
    bitarr->num_of_words = num_of_words;
    bitarr->num_of_bits = num_of_words * 64;
}

// FillAction is fill with 0 or 1 or toggle
typedef enum {ZERO_REGION, FILL_REGION, SWAP_REGION} FillAction;

static inline void _set_region(bitarray_t *bitarr, bit_index_t start,
                               bit_index_t length, FillAction action)
{
    if(length == 0) return;

    word_addr_t first_word = bitset64_wrd(start);
    word_addr_t last_word = bitset64_wrd(start+length-1);
    word_offset_t foffset = bitset64_idx(start);
    word_offset_t loffset = bitset64_idx(start+length-1);

    if (first_word == last_word) {
        word_t mask = bitmask64(length) << foffset;

        switch (action) {
            case ZERO_REGION: bitarr->words[first_word] &= ~mask; break;
            case FILL_REGION: bitarr->words[first_word] |=  mask; break;
            case SWAP_REGION: bitarr->words[first_word] ^=  mask; break;
        }
    } else {
        // Set first word
        switch(action) {
            case ZERO_REGION: bitarr->words[first_word] &=  bitmask64(foffset); break;
            case FILL_REGION: bitarr->words[first_word] |= ~bitmask64(foffset); break;
            case SWAP_REGION: bitarr->words[first_word] ^= ~bitmask64(foffset); break;
        }

        word_addr_t i;

        // Set whole words
        switch (action) {
            case ZERO_REGION:
                for(i = first_word + 1; i < last_word; i++)
                bitarr->words[i] = (word_t)0;
                break;
            case FILL_REGION:
                for(i = first_word + 1; i < last_word; i++)
                bitarr->words[i] = WORD_MAX;
                break;
            case SWAP_REGION:
                for(i = first_word + 1; i < last_word; i++)
                bitarr->words[i] ^= WORD_MAX;
                break;
        }

        // Set last word
        switch (action) {
            case ZERO_REGION: bitarr->words[last_word] &= ~bitmask64(loffset+1); break;
            case FILL_REGION: bitarr->words[last_word] |=  bitmask64(loffset+1); break;
            case SWAP_REGION: bitarr->words[last_word] ^=  bitmask64(loffset+1); break;
        }
    }
}

//
// Set, clear and toggle all bits in a region
//

// Set all the bits in a region
void bitarray_set_region(bitarray_t* bitarr, bit_index_t start, bit_index_t len)
{
    // assert(start + len <= bitarr->num_of_bits);
    SET_REGION(bitarr, start, len);
}


// Clear all the bits in a region
void bitarray_clear_region(bitarray_t* bitarr, bit_index_t start, bit_index_t len)
{
    // assert(start + len <= bitarr->num_of_bits);
    CLEAR_REGION(bitarr, start, len);
}

// Toggle all the bits in a region
void bitarray_toggle_region(bitarray_t* bitarr, bit_index_t start, bit_index_t len)
{
    // assert(start + len <= bitarr->num_of_bits);
    TOGGLE_REGION(bitarr, start, len);
}