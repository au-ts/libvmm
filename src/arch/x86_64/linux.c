/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <libvmm/dtb.h>
#include <libvmm/linux.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/linux.h>

uintptr_t linux_setup_images(uintptr_t ram_start,
                             size_t ram_size,
                             uintptr_t kernel,
                             size_t kernel_size,
                             uintptr_t initrd_src,
                             uintptr_t initrd_dest,
                             size_t initrd_size)
{
    return 0;
}
