/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <string.h>
#include <libvmm/dtb.h>
#include <libvmm/guest_ram.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/aarch64/linux.h>

uint64_t linux_setup_images(uint64_t ram_start_gpa, uintptr_t kernel, size_t kernel_size, uintptr_t dtb_src,
                            uint64_t dtb_dest_gpa, size_t dtb_size, uintptr_t initrd_src, uint64_t initrd_dest_gpa,
                            size_t initrd_size)
{
    // Before doing any copying, let's sanity check the addresses.
    // Check that the kernel and DTB to do not overlap
    uint64_t dtb_start = dtb_dest_gpa;
    uint64_t dtb_end = dtb_start + dtb_size;
    uint64_t kernel_start = kernel;
    uint64_t kernel_end = kernel_start + kernel_size;
    if (!(dtb_start >= kernel_end || dtb_end <= kernel_start)) {
        LOG_VMM_ERR("Linux kernel image [0x%lx..0x%lx)"
                    " overlaps with the destination of the DTB [0x%lx, 0x%lx)\n",
                    kernel_start, kernel_end, dtb_start, dtb_end);
        return 0;
    }
    // Check that the kernel and initrd do not overlap
    uint64_t initrd_start = initrd_dest_gpa;
    uint64_t initrd_end = initrd_start + initrd_size;
    if (!(initrd_start >= kernel_end || initrd_end <= kernel_start)) {
        LOG_VMM_ERR("Linux kernel image [0x%lx..0x%lx) overlaps"
                    " with the destination of the initial RAM disk [0x%lx, 0x%lx)\n",
                    kernel_start, kernel_end, initrd_start, initrd_end);
        return 0;
    }
    // Check that the DTB and initrd do not overlap
    if (!(initrd_start >= dtb_end || initrd_end <= dtb_start)) {
        LOG_VMM_ERR("DTB [0x%lx..0x%lx) overlaps with the destination of the"
                    " initial RAM disk [0x%lx, 0x%lx)\n",
                    dtb_start, dtb_end, initrd_start, initrd_end);
        return 0;
    }

    // First we inspect the kernel image header to confirm it is a valid image
    // and to determine where in memory to place the image.
    // The pointer to the kernel image may be unaligned, so to avoid undefined
    // behaviour, we do an explicit copy.
    struct linux_image_header image_header = {};
    memcpy((char *)&image_header, (char *)kernel, sizeof(struct linux_image_header));
    if (image_header.magic != LINUX_IMAGE_MAGIC) {
        LOG_VMM_ERR("Linux kernel image magic check failed\n");
        return 0;
    }

    // Check that the kernel image destination is in valid guest RAM, then copy
    size_t bytes_remaining;
    uint64_t kernel_dest_gpa = ram_start_gpa + image_header.text_offset;
    void *kernel_dest_vmm = gpa_to_vaddr(kernel_dest_gpa, &bytes_remaining);
    if (kernel_dest_vmm == NULL) {
        LOG_VMM_ERR("Linux kernel copy destination 0x%lx not in valid guest RAM\n", kernel_dest_gpa);
        return 0;
    }
    if (bytes_remaining < kernel_size) {
        LOG_VMM_ERR("Not enough guest RAM to copy Linux kernel\n");
        return 0;
    }

    // This check is because the Linux kernel image requires to be placed at text_offset of
    // a 2MB aligned base address anywhere in usable system RAM and called there.
    // In this case, we place the image at the text_offset of the start of the guest's RAM,
    // so we need to make sure that the start of guest RAM is 2MiB aligned.
    if ((ram_start_gpa & ((1 << 20) - 1)) != 0) {
        LOG_VMM_ERR("Linux requires that guest RAM starting GPA must be 2MiB aligned, got 0x%lx\n", ram_start_gpa);
        return 0;
    }
    LOG_VMM("Copying guest kernel image to 0x%lx (0x%lx bytes)\n", (uintptr_t)kernel_dest_vmm, kernel_size);
    memcpy((char *)kernel_dest_vmm, (char *)kernel, kernel_size);

    // Copy the guest device tree blob into the right location
    // First check that the DTB given is actually a DTB!
    struct dtb_header dtb_header = {};
    memcpy((char *)&dtb_header, (char *)dtb_src, sizeof(struct dtb_header));
    if (!dtb_check_magic(&dtb_header)) {
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
    assert(dtb_dest_gpa % 0x8 == 0);
    if (dtb_dest_gpa % 0x8) {
        LOG_VMM_ERR("Linux expects DTB address to be on an 8-byte boundary, DTB address is 0x%lx\n", dtb_dest_gpa);
        return 0;
    }

    void *dtb_dest_vmm = gpa_to_vaddr(dtb_dest_gpa, &bytes_remaining);
    if (dtb_dest_vmm == NULL) {
        LOG_VMM_ERR("DTB copy destination 0x%lx not in valid guest RAM\n", dtb_dest_gpa);
        return 0;
    }
    if (bytes_remaining < dtb_size) {
        LOG_VMM_ERR("Not enough guest RAM to copy DTB\n");
        return 0;
    }
    LOG_VMM("Copying guest DTB to GPA 0x%lx (0x%zx bytes)\n", dtb_dest_gpa, dtb_size);
    memcpy((char *)dtb_dest_vmm, (char *)dtb_src, dtb_size);

    // Copy the initial RAM disk into the right location
    // @ivanv: add checks for initrd according to Linux docs
    void *initrd_dest_vmm = gpa_to_vaddr(initrd_dest_gpa, &bytes_remaining);
    if (initrd_dest_vmm == NULL) {
        LOG_VMM_ERR("Initrd copy destination 0x%lx not in valid guest RAM\n", initrd_dest_gpa);
        return 0;
    }
    if (bytes_remaining < initrd_size) {
        LOG_VMM_ERR("Not enough guest RAM to copy initrd\n");
        return 0;
    }
    LOG_VMM("Copying guest initial RAM disk to GPA 0x%lx (0x%zx bytes)\n", (uintptr_t)initrd_dest_vmm, initrd_size);
    memcpy((char *)initrd_dest_vmm, (char *)initrd_src, initrd_size);

    return kernel_dest_gpa;
}
