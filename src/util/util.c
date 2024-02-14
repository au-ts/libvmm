/*
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "util.h"

uintptr_t uart_base;
#define UART_REG(x) ((volatile uint32_t *)(uart_base + (x)))

/* This is required to use the printf library we brought in, it is
   simply for convenience since there's a lot of logging/debug printing
   in the VMM. */
void _putchar(char character)
{
#ifdef CONFIG_PRINTING
    microkit_dbg_putc(character);
#else
#define UART_WFIFO 0x0
#define UART_STATUS 0xC
#define UART_TX_FULL (1 << 21)

    while ((*UART_REG(UART_STATUS) & UART_TX_FULL))
        ;
    *UART_REG(UART_WFIFO) = character;
#endif
}