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

#if defined(BOARD_odroidc4)
#define UART_WFIFO 0x0
#define UART_STATUS 0xC
#define UART_TX_FULL (1 << 21)

    while ((*UART_REG(UART_STATUS) & UART_TX_FULL))
        ;
    *UART_REG(UART_WFIFO) = character;
#elif defined(BOARD_qemu_arm_virt)
// qemu_arm_virt printing during benchmark mode
#define UART_BASE 0x9000000
#define UARTDR 0x000
#define UARTFR 0x018
#define PL011_UARTFR_TXFF (1 << 5)

    while ((*UART_REG(UARTFR) & PL011_UARTFR_TXFF) != 0)
        ;
    *UART_REG(UARTDR) = character;
#elif defined(BOARD_maaxboard)
#define UART_BASE 0x30860000
#define STAT 0x98
#define TRANSMIT 0x40
#define STAT_TDRE (1 << 14)
    while (!(*UART_REG(STAT) & STAT_TDRE))
    {
    }
    *UART_REG(TRANSMIT) = character;
#endif

#endif
}