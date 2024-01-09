#pragma once

#include <stdint.h>
#include <stdbool.h>

// 64 bit words
typedef uint64_t word_t, word_addr_t, bit_index_t;
typedef uint8_t word_offset_t; // Offset within a 64 bit word

#define BIT_INDEX_MIN 0
#define BIT_INDEX_MAX (~(bit_index_t)0)

#define roundup_bits2bytes(bits)   (((bits)+7)/8)
#define roundup_bits2words32(bits) (((bits)+31)/32)
#define roundup_bits2words64(bits) (((bits)+63)/64)

typedef struct bitarray {
  word_t* words; /* Word array */
  bit_index_t num_of_bits; /* Number of bits, calculated by number of words * size of word */
  word_addr_t num_of_words; /* Number of words */
} bitarray_t;

/* Initialise a bit array */
void bitarray_init(bitarray_t *bitarr, word_t *words, word_addr_t num_of_words);

char bitarray_get_bit(bitarray_t *bitarr, bit_index_t index);

/* Set all the bits in a region */
void bitarray_set_region(bitarray_t* bitarr, bit_index_t start, bit_index_t len);

/* Toggle all the bits in a region */
void bitarray_toggle_region(bitarray_t* bitarr, bit_index_t start, bit_index_t len);

/* Clear all the bits in a region */
void bitarray_clear_region(bitarray_t* bitarr, bit_index_t start, bit_index_t len);

/* Compare two bit arrays */
bool bitarray_cmp_region(bitarray_t* bitarr1, bit_index_t start1, bitarray_t* bitarr2, bit_index_t start2, bit_index_t len);