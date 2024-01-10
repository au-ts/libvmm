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

char bitarray_get_bit(bitarray_t *bitarr, bit_index_t index)
{
    word_addr_t word = bitset64_wrd(index);
    word_offset_t offset = bitset64_idx(index);
    return (bitarr->words[word] >> offset) & 1;
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

bool bitarray_cmp_region(bitarray_t* bitarr1, bit_index_t start1,
                         bitarray_t* bitarr2, bit_index_t start2, bit_index_t len) {
    // Check if the length exceeds the bounds of the bitarrays
    // assert(start1 + len > bitarr1->num_of_bits);
    // assert(start2 + len > bitarr2->num_of_bits);

    if (len == 0) return true;

    word_addr_t first_word1 = bitset64_wrd(start1);
    word_addr_t last_word1 = bitset64_wrd(start1 + len - 1);
    word_offset_t foffset1 = bitset64_idx(start1);
    word_offset_t loffset1 = bitset64_idx(start1 + len - 1);

    word_addr_t first_word2 = bitset64_wrd(start2);
    word_addr_t last_word2 = bitset64_wrd(start2 + len - 1);
    word_offset_t foffset2 = bitset64_idx(start2);
    word_offset_t loffset2 = bitset64_idx(start2 + len - 1);

    // Compare first words if necessary
    if (first_word1 == last_word1 && first_word2 == last_word2) {
        word_t mask1 = bitmask64(len) << foffset1;
        word_t mask2 = bitmask64(len) << foffset2;
        return ((bitarr1->words[first_word1] & mask1) == (bitarr2->words[first_word2] & mask2));
    } else {
        // Compare first words separately
        word_t first_mask1 = (~(bitmask64(foffset1))) & (bitmask64(64 - (foffset1 - loffset1 + 1)));
        word_t first_mask2 = (~(bitmask64(foffset2))) & (bitmask64(64 - (foffset2 - loffset2 + 1)));
        if ((bitarr1->words[first_word1] & first_mask1) != (bitarr2->words[first_word2] & first_mask2)) {
            return false;
        }

        // Compare whole words in between
        for (word_addr_t i = 1; i < last_word1 - first_word1; i++) {
            if (bitarr1->words[first_word1 + i] != bitarr2->words[first_word2 + i]) {
                return false;
            }
        }

        // Compare the last words if necessary
        if (last_word1 != first_word1 && last_word2 != first_word2) {
            word_t last_mask1 = bitmask64(loffset1 + 1);
            word_t last_mask2 = bitmask64(loffset2 + 1);
            if ((bitarr1->words[last_word1] & last_mask1) != (bitarr2->words[last_word2] & last_mask2)) {
                return false;
            }
        }
    }

    return true;
}


