/*
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/util/util.h>
#include <libvmm/util/printf.h>

/* This is required to use the printf library we brought in, it is
   simply for convenience since there's a lot of logging/debug printing
   in the VMM. */
void _putchar(char character)
{
    microkit_dbg_putc(character);
}

void print_mem_hex(uintptr_t addr, size_t size)
{
#ifdef CONFIG_DEBUG_BUILD
    for (size_t i = 0; i < size; i++) {
        printf("%02X ", *((uint8_t *)addr + i));
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
#endif
}
