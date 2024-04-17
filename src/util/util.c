/*
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "util.h"
#include "printf.h"

/* This is required to use the printf library we brought in, it is
   simply for convenience since there's a lot of logging/debug printing
   in the VMM. */
void _putchar(char character)
{
    microkit_dbg_putc(character);
}

void print_bitarray(bitarray_t* bitarr) {
#ifdef CONFIG_DEBUG_BUILD
    for (int i = 0; i < bitarr->num_of_words; i++)
    {
        printf("%d:", i);
        for (int j = 0; j < sizeof(bitarr->words[i]) * 8; j++)
        {
            printf("%lu", (bitarr->words[i] >> j) & 1);
        }
        printf("\n");
    }
#endif
}

void print_binary(word_t word) {
#ifdef CONFIG_DEBUG_BUILD
    for (int i = 63; i >= 0; i--) {
        printf("%llu", (word >> i) & 1);

        if (i % 8 == 0 && i != 0) {
            printf(" ");
        }
    }
    printf("\n");
#endif
void *memset(void *dest, int c, size_t n)
{
    unsigned char *s = dest;
    for (; n; n--, s++) *s = c;
    return dest;
}

void print_mem_hex(uintptr_t addr, size_t size) {
#ifdef CONFIG_DEBUG_BUILD
    for (size_t i = 0; i < size; i++) {
        printf("%02X ", *((uint8_t*)addr + i));
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
#endif
}
