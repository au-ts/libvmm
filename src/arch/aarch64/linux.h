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

// Note that this function assumes that the `kernel` parameter is aligned
// to at least the alignment of `struct linux_image_header`. The `dtb_src`
// paramter must be aligned to at least the alignment of `struct dtb_header`.
uintptr_t linux_setup_images(uintptr_t ram_start,
                             uintptr_t ram_size,
                             uintptr_t kernel,
                             size_t kernel_size,
                             uintptr_t dtb_src,
                             uintptr_t dtb_dest,
                             size_t dtb_size,
                             uintptr_t initrd_src,
                             uintptr_t initrd_dest,
                             size_t initrd_size);
