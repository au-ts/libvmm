/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/dtb.h>
#include <libvmm/util/util.h>
#include <libvmm/linux.h>
#include <libvmm/arch/aarch64/linux.h>

uintptr_t linux_setup_images(uintptr_t ram_start,
                             size_t ram_size,
                             uintptr_t kernel,
                             size_t kernel_size,
                             uintptr_t dtb_src,
                             uintptr_t dtb_dest,
                             size_t dtb_size,
                             uintptr_t initrd_src,
                             uintptr_t initrd_dest,
                             size_t initrd_size)
{
    // First we inspect the kernel image header to confirm it is a valid image
    // and to determine where in memory to place the image.
    // The pointer to the kernel image may be unaligned, so to avoid undefined
    // behaviour, we do an explicit copy.
    struct linux_image_header image_header = {};
    memcpy((char *)&image_header, (char *)kernel, sizeof(struct linux_image_header));
    assert(image_header.magic == LINUX_IMAGE_MAGIC);
    if (image_header.magic != LINUX_IMAGE_MAGIC) {
        LOG_VMM_ERR("Linux kernel image magic check failed\n");
        return 0;
    }
    // Copy the guest kernel image into the right location
    uintptr_t kernel_dest = ram_start + image_header.text_offset;

    // Before doing any copying, validate the image destinations in respect to each other and RAM.
    if (!linux_validate_image_locations(ram_start, ram_size, kernel_dest, kernel_size, dtb_dest, dtb_size, initrd_dest,
                                        initrd_size)) {
        LOG_VMM_ERR("invalid image location given\n");
        return 0;
    }

    // This check is because the Linux kernel image requires to be placed at text_offset of
    // a 2MB aligned base address anywhere in usable system RAM and called there.
    // In this case, we place the image at the text_offset of the start of the guest's RAM,
    // so we need to make sure that the start of guest RAM is 2MiB aligned.
    assert((ram_start & ((1 << 20) - 1)) == 0);
    LOG_VMM("Copying guest kernel image to 0x%x (0x%x bytes)\n", kernel_dest, kernel_size);
    memcpy((char *)kernel_dest, (char *)kernel, kernel_size);
    // Copy the guest device tree blob into the right location
    // First check that the DTB given is actually a DTB!
    if (!dtb_check_magic((char *)dtb_src)) {
        LOG_VMM_ERR("Given DTB does not match DTB magic.\n");
        return 0;
    }
    // Linux does not allow the DTB to be greater than 2 megabytes in size.
    assert(dtb_size <= (1 << 21));
    if (dtb_size > (1 << 21)) {
        LOG_VMM_ERR("Linux expects size of DTB to be less than 2MB, DTB size is 0x%lx bytes\n", dtb_size);
        return 0;
    }
    // Linux expects the address of the DTB to be on an 8-byte boundary.
    assert(dtb_dest % 0x8 == 0);
    if (dtb_dest % 0x8) {
        LOG_VMM_ERR("Linux expects DTB address to be on an 8-byte boundary, DTB address is 0x%lx\n", dtb_dest);
        return 0;
    }
    LOG_VMM("Copying guest DTB to 0x%x (0x%x bytes)\n", dtb_dest, dtb_size);
    memcpy((char *)dtb_dest, (char *)dtb_src, dtb_size);
    // Copy the initial RAM disk into the right location
    // @ivanv: add checks for initrd according to Linux docs
    LOG_VMM("Copying guest initial RAM disk to 0x%x (0x%x bytes)\n", initrd_dest, initrd_size);
    memcpy((char *)initrd_dest, (char *)initrd_src, initrd_size);

    return kernel_dest;
}
