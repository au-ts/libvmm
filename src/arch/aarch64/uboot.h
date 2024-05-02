/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "fault.h"

/* A dummy struct for a pl011 device */
typedef struct pl011_device {
    // TODO -- fill out with device info for pl011
} pl011_device_t;

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

/*
    Atm this is jut going to register a fault handler - it'll probably need to do some init stuff for 
    the device eventually.
*/
bool pl011_emulation_init(uintptr_t base, size_t size, pl011_device_t *dev);

/*
    Atm this does nothing and just returns true.
*/
bool pl011_fault_handle();
