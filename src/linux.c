/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <libvmm/dtb.h>
#include <libvmm/linux.h>
#include <libvmm/util/util.h>

bool linux_validate_image_locations(uintptr_t ram_start,
                                    size_t ram_size,
                                    uintptr_t kernel,
                                    size_t kernel_size,
                                    uintptr_t dtb_dest,
                                    size_t dtb_size,
                                    uintptr_t initrd_dest,
                                    size_t initrd_size)
{
    uintptr_t ram_end = ram_start + ram_size;
    uintptr_t dtb_start = dtb_dest;
    uintptr_t dtb_end = dtb_start + dtb_size;
    uintptr_t kernel_start = kernel;
    uintptr_t kernel_end = kernel_start + kernel_size;
    uintptr_t initrd_start = initrd_dest;
    uintptr_t initrd_end = initrd_start + initrd_size;

    if (!(kernel_start >= ram_start && kernel_end <= ram_end)) {
        LOG_VMM_ERR("kernel image [0x%lx..0x%lx) does not reside within RAM [0x%lx, 0x%lx)\n",
                    kernel_start, kernel_end, ram_start, ram_end);
        return false;
    }

    if (!(dtb_start >= ram_start && dtb_end <= ram_end)) {
        LOG_VMM_ERR("DTB [0x%lx..0x%lx) does not reside within RAM [0x%lx, 0x%lx)\n",
                    dtb_start, dtb_end, ram_start, ram_end);
        return false;
    }

    if (!(initrd_start >= ram_start && initrd_end <= ram_end)) {
        LOG_VMM_ERR("initial RAM disk image [0x%lx..0x%lx) does not reside within RAM [0x%lx, 0x%lx)\n",
                    initrd_start, initrd_end, ram_start, ram_end);
        return false;
    }

    // Check that the kernel and DTB to do not overlap
    if (!(dtb_start >= kernel_end || dtb_end <= kernel_start)) {
        LOG_VMM_ERR("Linux kernel image [0x%lx..0x%lx)"
                    " overlaps with the destination of the DTB [0x%lx, 0x%lx)\n",
                    kernel_start, kernel_end, dtb_start, dtb_end);
        return false;
    }

    if (!(initrd_start >= kernel_end || initrd_end <= kernel_start)) {
        LOG_VMM_ERR("kernel image [0x%lx..0x%lx) overlaps"
                    " with the destination of the initial RAM disk [0x%lx, 0x%lx)\n",
                    kernel_start, kernel_end, initrd_start, initrd_end);
        return false;
    }
    // Check that the DTB and initrd do not overlap
    if (!(initrd_start >= dtb_end || initrd_end <= dtb_start)) {
        LOG_VMM_ERR("DTB [0x%lx..0x%lx) overlaps with the destination of the"
                    " initial RAM disk [0x%lx, 0x%lx)\n",
                    dtb_start, dtb_end, initrd_start, initrd_end);
        return false;
    }

    return true;
}
