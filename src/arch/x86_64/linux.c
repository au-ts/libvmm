/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <sddf/util/custom_libc/string.h>
#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/linux.h>

#define HEADER_SECTOR_SIZE 512

#define HEADER_MAGIC 0x53726448

/* We do not have an assigned ID in the table of loader IDs, so we do 0xff. */
#define TYPE_OF_LOADER 0xff

#define HEADER_SETUP_SECTS_OFFSET 0x1f1
#define HEADER_MAGIC_OFFSET 0x202
#define HEADER_PROTOCOL_VERSION_OFFSET 0x206
#define HEADER_KERNEL_VERSION_OFFSET 0x20e
#define HEADER_TYPE_OF_LOADER_OFFSET 0x210
#define HEADER_LOADFLAGS_OFFSET 0x211
#define HEADER_TYPE_RAM_DISK_IMAGE_OFFSET 0x218
#define HEADER_TYPE_RAM_DISK_SIZE_OFFSET 0x21c
#define HEADER_HEAP_END_PTR_OFFSET 0x224
#define HEADER_CMD_LINE_PTR_OFFSET 0x228

#define LOADFLAGS_CAN_USE_HEAP (1 << 7)

#define INITRD_DEST 0x10100000

#define KERNEL_DEST 0x80000000

uintptr_t linux_setup_images(uintptr_t ram_start,
                             size_t ram_size,
                             uintptr_t kernel,
                             size_t kernel_size,
                             uintptr_t initrd_src,
                             size_t initrd_size,
                             char *cmdline)
{
    uint32_t magic = *(uint32_t *)((char *)kernel + HEADER_MAGIC_OFFSET);
    if (magic != HEADER_MAGIC) {
        LOG_VMM_ERR("invalid Linux kernel magic or boot protocol too old (< 2.0)\n");
        return 0;
    }

    uint16_t protocol_version = *(uint16_t *)((char *)kernel + HEADER_PROTOCOL_VERSION_OFFSET);
    uint8_t protocol_version_major = protocol_version >> 8;
    uint8_t protocol_version_minor = protocol_version & 0xff;
    LOG_VMM("Linux boot protocol %d.%d\n", protocol_version_major, protocol_version_minor);

    uint16_t kernel_version_offset = *(uint16_t *)((char *)kernel + HEADER_KERNEL_VERSION_OFFSET);
    // TODO: null terminate check and check that setup sects is valid
    char *kernel_version = (char *)(kernel + kernel_version_offset + 0x200);
    LOG_VMM("Linux kernel version string: %s\n", kernel_version);

    uint8_t loadflags = *(uint8_t *)(kernel + HEADER_LOADFLAGS_OFFSET);
    LOG_VMM("loadflags: 0x%x\n", loadflags);

    uintptr_t base_ptr = 0x90000;
    char *kernel_real_mode_dest = KERNEL_DEST + (char *)base_ptr;

    // Copy real mode part of kernel
    memcpy(kernel_real_mode_dest, (char *)kernel, 0x7fff);

    // Obligatory fields to fill out

    char *setup = kernel_real_mode_dest;

    uint8_t setup_sects = *(uint8_t *)((char *)kernel + HEADER_SETUP_SECTS_OFFSET);
    if (setup_sects == 0) {
        setup_sects = 4;
    }

    // vid_mode

    // type_of_loader
    setup[HEADER_TYPE_OF_LOADER_OFFSET] = TYPE_OF_LOADER;

    // setup_move_size
    // TODO: we assume protocol is not 2.00 or 2.01 so we do not set setup_move_size
    assert(protocol_version >= 0x0202);

    // ramdisk_image
    *(uint32_t *)(setup + HEADER_TYPE_RAM_DISK_IMAGE_OFFSET) = INITRD_DEST;
    // ramdisk_size
    *(uint32_t *)(setup + HEADER_TYPE_RAM_DISK_SIZE_OFFSET) = initrd_size;
    // heap_end_ptr
    uint16_t heap_end;
    if (protocol_version >= 0x0202 && loadflags & 0x1) {
        heap_end = base_ptr + 0xe000;
    } else {
        heap_end = base_ptr + 0x9800;
    }

    LOG_VMM("heap_end: 0x%lx\n", heap_end);

    if (protocol_version >= 0x201) {
        *(uint16_t *)(setup + HEADER_HEAP_END_PTR_OFFSET) = heap_end - 0x200;
        setup[HEADER_LOADFLAGS_OFFSET] = loadflags | LOADFLAGS_CAN_USE_HEAP;
    }

    // cmd_line_ptr
    if (protocol_version >= 0x202) {
        // TODO: check that length of command line is valid;
        char *cmdline_dest = setup + heap_end;
        strcpy(cmdline_dest, cmdline);
        *(uint32_t *)(setup + HEADER_CMD_LINE_PTR_OFFSET) = heap_end;
        LOG_VMM("command line: '%s'\n", cmdline_dest);
    } else {
        assert(false);
    }

    // We have setup the header now. So all that is left to do is
    // copy the kernel and initrd to the right place.
    bool is_bzImage = (protocol_version >= 0x0200) && (loadflags & 0x01);
    uintptr_t load_address = is_bzImage ? 0x100000 : 0x10000;
    assert(load_address == 0x100000);

    // Copy non-real mode part of kernel
    size_t non_real_mode_offset = ((setup_sects + 1) * HEADER_SECTOR_SIZE);
    char *non_real_mode_bytes = (char *)kernel + non_real_mode_offset;
    size_t non_real_mode_size = kernel_size - non_real_mode_offset;
    memcpy(kernel_real_mode_dest + load_address, non_real_mode_bytes, non_real_mode_size);
    LOG_VMM("non-real-mode load_address: 0x%lx, size of non-real mode: 0x%lx\n", load_address, non_real_mode_size);

    uintptr_t seg = base_ptr >> 4;

    // TODO: handle error
    vcpu_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_SS_SELECTOR, seg);
    vcpu_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_ES_SELECTOR, seg);
    vcpu_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_DS_SELECTOR, seg);
    vcpu_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_FS_SELECTOR, seg);
    vcpu_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_GS_SELECTOR, seg);
    vcpu_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_RIP, seg + 0x20);
    vcpu_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_RSP, 0x99000);

    return seg + 0x20;
}