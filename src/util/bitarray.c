#include "bitarray.h"

#define WORD_MAX  (~(word_t)0)

#define bitmask(nbits,type) ((nbits) ? ~(type)0 >> (sizeof(type)*8-(nbits)): (type)0)
#define bitmask32(nbits) bitmask(nbits,uint32_t)
#define bitmask64(nbits) bitmask(nbits,uint64_t)

#define bitset64_wrd(bit_pos) ((bit_pos) >> 6)
#define bitset64_idx(bit_pos) ((bit_pos) & 63)

/**
 * Set a region of bits in a bit array to a specified value.
 *
 * @param arr The bit array.
 * @param start The starting position of the region.
 * @param len The length of the region.
 */
#define SET_REGION(arr,start,len) _set_region((arr),(start),(len),FILL_REGION)

/**
 * Clear a region of bits in a bit array, setting them to 0.
 *
 * @param arr The bit array.
 * @param start The starting position of the region.
 * @param len The length of the region.
 */
#define CLEAR_REGION(arr,start,len) _set_region((arr),(start),(len),ZERO_REGION)

/**
 * Toggle a region of bits in a bit array, flipping their values.
 *
 * @param arr The bit array.
 * @param start The starting position of the region.
 * @param len The length of the region.
 */
#define TOGGLE_REGION(arr,start,len) _set_region((arr),(start),(len),SWAP_REGION)

void bitarray_init(bitarray_t *bitarr, word_t *words, word_index_t num_of_words)
{
    bitarr->words = words;
    bitarr->num_of_words = num_of_words;
    bitarr->num_of_bits = num_of_words * 64;
}

char bitarray_get_bit(bitarray_t *bitarr, bit_index_t index)
{
    word_index_t word = bitset64_wrd(index);
    word_offset_t offset = bitset64_idx(index);
    return (bitarr->words[word] >> offset) & 1;
}

// FillAction is fill with 0 or 1 or toggle
typedef enum {ZERO_REGION, FILL_REGION, SWAP_REGION} FillAction;

static inline void _set_region(bitarray_t *bitarr, bit_index_t start,
                               bit_index_t length, FillAction action)
{
    if(length == 0) return;

    word_index_t first_word = bitset64_wrd(start);
    word_index_t last_word = bitset64_wrd(start+length-1);
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

        word_index_t i;

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

void bitarray_set_region(bitarray_t* bitarr, bit_index_t start, bit_index_t len)
{
    // assert(start + len <= bitarr->num_of_bits);
    SET_REGION(bitarr, start, len);
}

void bitarray_clear_region(bitarray_t* bitarr, bit_index_t start, bit_index_t len)
{
    // assert(start + len <= bitarr->num_of_bits);
    CLEAR_REGION(bitarr, start, len);
}

void bitarray_toggle_region(bitarray_t* bitarr, bit_index_t start, bit_index_t len)
{
    // assert(start + len <= bitarr->num_of_bits);
    TOGGLE_REGION(bitarr, start, len);
}

bool bitarray_cmp_region(bitarray_t* bitarr1, bit_index_t start1,
                         bitarray_t* bitarr2, bit_index_t start2, bit_index_t len)
{
    if (len == 0) return true;

    while (len > 0) {
        // calculate the word index and bit offset for both arrays
        word_index_t word_idx1 = bitset64_wrd(start1);
        word_offset_t bit_offset1 = bitset64_idx(start1);
        word_index_t word_idx2 = bitset64_wrd(start2);
        word_offset_t bit_offset2 = bitset64_idx(start2);

        // calculate the number of bits to compare in this iteration
        bit_index_t bits_in_current_word1 = 64 - bit_offset1;
        bit_index_t bits_in_current_word2 = 64 - bit_offset2;
        bit_index_t bits_to_compare = len;
        if (bits_to_compare > bits_in_current_word1) bits_to_compare = bits_in_current_word1;
        if (bits_to_compare > bits_in_current_word2) bits_to_compare = bits_in_current_word2;

        // create masks for the bits to compare
        word_t mask1 = bitmask64(bits_to_compare) << bit_offset1;
        word_t mask2 = bitmask64(bits_to_compare) << bit_offset2;

        // extract the relevant bits from each array
        word_t bits1 = (bitarr1->words[word_idx1] & mask1) >> bit_offset1;
        word_t bits2 = (bitarr2->words[word_idx2] & mask2) >> bit_offset2;

        // compare the bits
        if (bits1 != bits2) {
            return false;
        }

        // update for the next iteration
        len -= bits_to_compare;
        start1 += bits_to_compare;
        start2 += bits_to_compare;
    }

    return true;
}

