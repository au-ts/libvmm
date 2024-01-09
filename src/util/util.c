/*
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "util.h"

/* This is required to use the printf library we brought in, it is
   simply for convenience since there's a lot of logging/debug printing
   in the VMM. */
void _putchar(char character)
{
    microkit_dbg_putc(character);
}

/* Convenience function to print a bit array */
void print_bitarray(bitarray_t* bitarr)
{
    for (int i = 0; i < bitarr->num_of_words; i++)
    {
        printf("%d:", i);
        for (int j = 0; j < sizeof(bitarr->words[i]) * 8; j++)
        {
            printf("%lu", (bitarr->words[i] >> j) & 1);
        }
        printf("\n");
    }
}