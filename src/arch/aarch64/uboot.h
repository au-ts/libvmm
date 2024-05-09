/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "fault.h"

/* Offsets from base address for PL011 registers */
static const uint32_t DR_OFFSET = 0x000;
static const uint32_t FR_OFFSET = 0x018;
static const uint32_t IBRD_OFFSET = 0x024;
static const uint32_t FBRD_OFFSET = 0x028;
static const uint32_t LCR_OFFSET = 0x02c;
static const uint32_t TCR_OFFSET = 0x030;
static const uint32_t IMSC_OFFSET = 0x038;
static const uint32_t DMACR_OFFSET = 0x048;

/* PL011 register set */
typedef struct pl011_registers {
    /* Note that all the registers are 32 bits wide but only some of these bits
    are actually used (the lower bits) */
    uint32_t dr;        /* 0x000 Data Register */
    uint32_t rsr_ecr;   /* 0x004 Receive Status Register/Error Clear Register */
    uint32_t res1;      /* 0x008 Reserved */
    uint32_t res2;      /* 0x00c Reserved */
    uint32_t res3;      /* 0x010 Reserved */
    uint32_t res4;      /* 0x014 Reserved */
    uint32_t fr;        /* 0x018 Flag Register */
    uint32_t res5;      /* 0x01c Reserved */
    uint32_t ilpr;      /* 0x020 IrDA Low-Power Counter Register */
    uint32_t ibrd;      /* 0x024 Integer Baud Rate Register */
    uint32_t fbrd;      /* 0x028 Fractional Baud Rate Register */
    uint32_t lcr_h;     /* 0x02c Line Control Register */
    uint32_t tcr;       /* 0x030 Control Register */
    uint32_t ifls;      /* 0x034 Interrupt FIFO Level Select Register */
    uint32_t imsc;      /* 0x038 Interrupt Mask Set/Clear Register */
    uint32_t ris;       /* 0x03C Raw Interrupt Status Register */
    uint32_t mis;       /* 0x040 Masked Interrupt Status Register */
    uint32_t icr;       /* 0x044 Interrupt Clear Register */
    uint32_t dmacr;     /* 0x048 DMA Control Register */  
} pl011_registers_t;

/* PL011 device information */
typedef struct pl011_device {
    uint64_t base_address;
    uint64_t base_clock;
    uint32_t baudrate;
    uint32_t data_bits;
    uint32_t stop_bits;
} pl011_device_t;

/* Function for accessing specific registers */
// volatile uint32_t *reg(const pl011_device_t *dev, uint32_t offset) {
//     const uint64_t addr = dev->base_address + offset;

//      // Returning a pointer to a locally defined variable???
//     return (volatile uint32_t *) ((void *) addr);
// }

/* Calculations for clock */
// static void calculate_divisors(const pl011_device_t *dev, uint32_t *integer, uint32_t *fractional) {
//     const uint32_t div = 4 * dev->base_clock / dev->baudrate;

//     *fractional = div & 0x3f;
//     *integer = (div >> 6) & 0xffff;
// }

// int pl011_device_setup(pl011_device_t *dev, uint64_t base_address, uint64_t base_clock) {

// }

/*
    Atm this is jut going to register a fault handler - it'll probably need to do some init stuff for 
    the device eventually.
*/
bool pl011_emulation_init(uintptr_t base, size_t size, pl011_device_t *dev);

/*
    This passes the fault off to the correct handler depending on if read or right. Note that atm we're 
    not actually using the data passed in (we're using a global pl011 device so don't need to pass it around).
*/
bool pl011_fault_handle(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data);

/* 
    ram_start: this is set by the microkit and is the start of the guest RAM memory region
    uboot: this is filled in via the package_guest_image.S file and is the uboot binary
    uboot_size: we can calculate this and its just how large the uboot binary is
*/
uintptr_t uboot_setup_image(uintptr_t ram_start,
                             uintptr_t uboot,
                             size_t uboot_size,
                             size_t uboot_offset,
                             uintptr_t dtb_src,
                             uintptr_t dtb_dest,
                             size_t dtb_size
                             );

bool uboot_start(size_t boot_vcpu_id, uintptr_t pc, uintptr_t dtb);
