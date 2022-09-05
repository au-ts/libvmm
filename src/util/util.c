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
    sel4cp_dbg_putc(character);
}

 __attribute__ ((__noreturn__))
void __assert_func(const char *file, int line, const char *function, const char *str)
{
    sel4cp_dbg_puts("assert failed: ");
    sel4cp_dbg_puts(str);
    sel4cp_dbg_puts(" ");
    sel4cp_dbg_puts(file);
    sel4cp_dbg_puts(" ");
    sel4cp_dbg_puts(function);
    sel4cp_dbg_puts("\n");
    while (1) {}
}
