/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

/*
 * The structure of the kernel image header and the magic value comes from the
 * Linux kernel documentation that can be found in the Linux source code. The
 * path is Documentation/arm64/booting.rst or Documentation/arch/arm64/booting.rst
 * depending on the version of the source code.
 */

#define LINUX_IMAGE_MAGIC 0x644d5241

struct linux_image_header {
    uint32_t code0;       // Executable code
    uint32_t code1;       // Executable code
    uint64_t text_offset; // Image load offset, little endian
    uint64_t image_size;  // Effective Image size, little endian
    uint64_t flags;       // kernel flags, little endian
    uint64_t res2;        // reserved
    uint64_t res3;        // reserved
    uint64_t res4;        // reserved
    uint32_t magic;       // Magic number, little endian, "ARM\x64"
    uint32_t res5;        // reserved (used for PE COFF offset)
};

/* 
    ram_start: this is set by the microkit and is the start of the guest RAM memory region
    uboot: this is filled in via the package_guest_image.S file and is the uboot binary
    uboot_size: we can calculate this and its just how large the uboot binary is

    Note: we're not providing a device tree here though we might have to? Will have to see.
*/
uintptr_t uboot_setup_images(uintptr_t ram_start,
                             uintptr_t uboot,
                             size_t uboot_size,
                             uintptr_t dtb_src,
                             uintptr_t dtb_dest,
                             size_t dtb_size
                             );

uintptr_t uboot_setup_kernel(uintptr_t ram_start,
                             uintptr_t uboot,
                             size_t uboot_size,
                             uintptr_t dtb_src,
                             uintptr_t dtb_dest,
                             size_t dtb_size,
                             uintptr_t image_src,
                             uintptr_t image_dest,
                             size_t image_size
                             );

bool uboot_start(size_t boot_vcpu_id, uintptr_t pc, uintptr_t dtb);
