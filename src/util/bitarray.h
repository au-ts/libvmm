#pragma once

#include <stdint.h>
#include <stdbool.h>

/**
 * This file provides functions and macros to manipulate bit arrays efficiently.
 * In a bit array, the bits are stored in a sequence of words, where each word is typically a machine word
 * (e.g., 32 bits or 64 bits). This implementation uses 64 bits.
 */

typedef uint64_t word_t; /* type of word in the bitarray. */
typedef uint64_t word_index_t; /* type for index of a word in the bitarray. */
typedef uint64_t bit_index_t; /* type for index of a bit in the bitarray. */
typedef uint8_t word_offset_t; /* type for offset of a bit within a word in the bitarray. */

/**
 * Rounds up the number of bits to the nearest number of bytes.
 *
 * @param bits The number of bits.
 * @return The number of bytes.
 */
#define roundup_bits2bytes(bits)   (((bits)+7)/8)

/**
 * Rounds up the number of bits to the nearest number of 32-bit words.
 *
 * @param bits The number of bits.
 * @return The number of 32-bit words.
 */
#define roundup_bits2words32(bits) (((bits)+31)/32)

/**
 * Rounds up the number of bits to the nearest number of 64-bit words.
 *
 * @param bits The number of bits.
 * @return The number of 64-bit words.
 */
#define roundup_bits2words64(bits) (((bits)+63)/64)

typedef struct bitarray {
  word_t* words; /* Word array */
  bit_index_t num_of_bits; /* Number of bits, calculated by number of words * size of word */
  word_index_t num_of_words; /* Number of words */
} bitarray_t;

/**
 * Initialise a bit array.
 * 
 * @param bitarr pointer to the bitarray struct.
 * @param words pointer to the word array.
 * @param num_of_words number of words in the word array.
 */
void bitarray_init(bitarray_t *bitarr, word_t *words, word_index_t num_of_words);

/**
 * Get the value of a specific bit in the bit array.
 * 
 * @param bitarr pointer to the bitarray struct.
 * @param index index of the bit to get.
 * @return the value of the bit (0 or 1).
 */
char bitarray_get_bit(bitarray_t *bitarr, bit_index_t index);

/**
 * Set all the bits in a specific region of the bit array.
 * 
 * @param bitarr pointer to the bitarray struct.
 * @param start starting index of the region.
 * @param len length of the region.
 */
void bitarray_set_region(bitarray_t* bitarr, bit_index_t start, bit_index_t len);

/**
 * Toggle all the bits in a specific region of the bit array.
 * 
 * @param bitarr pointer to the bitarray struct.
 * @param start starting index of the region.
 * @param len length of the region.
 */
void bitarray_toggle_region(bitarray_t* bitarr, bit_index_t start, bit_index_t len);

/**
 * Clear all the bits in a specific region of the bit array.
 * 
 * @param bitarr pointer to the bitarray struct.
 * @param start starting index of the region.
 * @param len length of the region.
 */
void bitarray_clear_region(bitarray_t* bitarr, bit_index_t start, bit_index_t len);

/**
 * Compare two regions of bit arrays.
 * 
 * @param bitarr1 pointer to the first bitarray struct.
 * @param start1 starting index of the first region.
 * @param bitarr2 pointer to the second bitarray struct.
 * @param start2 starting index of the second region.
 * @param len length of the regions to compare.
 * @return true if the two regions are equal, false otherwise.
 */
bool bitarray_cmp_region(bitarray_t* bitarr1, bit_index_t start1, bitarray_t* bitarr2, bit_index_t start2, bit_index_t len);