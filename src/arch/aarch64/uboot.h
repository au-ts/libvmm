/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

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
