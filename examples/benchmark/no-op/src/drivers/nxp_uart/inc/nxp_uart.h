/** 
 * Bao, a Lightweight Static Partitioning Hypervisor 
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details. 
 *
 */

#ifndef UART_NXP_H
#define UART_NXP_H

#include <stdint.h>
#include <stdbool.h>
#include <uart.h>

struct lpuart {
    uint32_t verid; 
    uint32_t param;
    uint32_t global;
    uint32_t pincfg;
    uint32_t baud;
    uint32_t stat;
    uint32_t ctrl;
    uint32_t data;
    uint32_t match;
    uint32_t modir;
    uint32_t fifo;
    uint32_t water;
};

#define LPUART_GLOBAL_RST_BIT  (1U << 1)
#define LPUART_BAUD_80MHZ_115200 ((4 << 24) | (1 << 17) | 138)
#define LPUART_CTRL_TE_BIT (1U << 19)
#define LPUART_CTRL_RE_BIT (1U << 18)
#define LPUART_CTRL_RIE_BIT (1U << 21)
#define LPUART_STAT_TDRE_BIT (1U << 23)
#define LPUART_STAT_OR_BIT (1U << 19)

void nxp_uart_init(volatile struct lpuart *uart);
void nxp_uart_putc(volatile struct lpuart *uart, char c);
char nxp_uart_getchar(volatile struct lpuart *uart);
void nxp_uart_enable_rxirq(volatile struct lpuart *uart);
void nxp_uart_clear_rxirq(volatile struct lpuart *uart);

#endif /* __UART_NXP_H */
