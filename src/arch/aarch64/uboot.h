/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "fault.h"
#include "virtio/mmio.h" // TODO - not sure I should be including this here but do atm cause it contains sddf structures
#include "virtio/console.h" // SAME AS ABOVE
#include <sddf/serial/queue.h> // SAME AS ABOVE

/* PL011 console internals */
#define PL011_CONSOLE_NUM_VIRTQ 2

/* PL011 device details given here - https://developer.arm.com/documentation/ddi0183/g/programmers-model */

/* PL011 register definitions */
#define PL011_FR_RXFE (1 << 4) 
#define PL011_FR_TXFF (1 << 5)

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

typedef struct pl011_queue_handler {
    struct virtq virtq;
    /* is this virtq fully initialised? */
    bool ready;
    /* the last index that the virtIO device processed */
    uint16_t last_idx;
} pl011_queue_handler_t;

/* PL011 device information */
typedef struct pl011_device {
    pl011_registers_t registers;
    uint64_t base_address;
    uint32_t size;
    sddf_handler_t *sddf_handlers;
    size_t num_vqs;
    pl011_queue_handler_t *vqs;
    
    /* Don't really need these as we're just emulating */
    // uint64_t base_clock;
    // uint32_t baudrate;
    // uint32_t data_bits;
    // uint32_t stop_bits;
} pl011_device_t;

/*
    This does some setup for the device and registers the fault handler.
*/
bool pl011_emulation_init(pl011_device_t *dev, uintptr_t base, size_t size, sddf_handler_t* sddf_handlers);

/*
    This passes the fault off to the correct handler depending on if read or right. Note that atm we're 
    not actually using the data passed in (we're using a global pl011 device so don't need to pass it around).
*/
bool pl011_fault_handle(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data);

/*
    Handles receive for the PL011 device.
*/
bool pl011_console_handle_rx(pl011_device_t *dev);

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
