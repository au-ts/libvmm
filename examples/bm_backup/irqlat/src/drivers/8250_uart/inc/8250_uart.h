/*
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * Copyright (c) 2019 Western Digital Corporation or its affiliates.
 *
 * Authors:
 *   Anup Patel <anup.patel@wdc.com>
 */

#ifndef __SERIAL_UART8250_H__
#define __SERIAL_UART8250_H__

//#include <sbi/sbi_types.h>
#include <stdint.h>
typedef uint32_t u32;
typedef uint16_t u16;

void uart8250_enable_rx_int();

void uart8250_putc(char ch);

int uart8250_getc(void);

int uart8250_init(unsigned long base, u32 in_freq, u32 baudrate, u32 reg_shift,
		  u32 reg_width);

void uart8250_interrupt_handler();

#endif
