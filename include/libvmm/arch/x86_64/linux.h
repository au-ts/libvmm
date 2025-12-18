/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Documents referenced.
// 1. https://www.kernel.org/doc/html/v6.1/x86/boot.html
// 2. Linux: arch/x86/include/uapi/asm/bootparam.h
// 3. Linux: include/uapi/linux/edd.h
// 4. Linux: arch/x86/include/uapi/asm/setup_data.h

// [2]
struct setup_header {
    uint8_t setup_sects;
    uint16_t root_flags;
    uint32_t syssize;
    uint16_t ram_size;
    uint16_t vid_mode;
    uint16_t root_dev;
    uint16_t boot_flag;
    uint16_t jump;
    uint32_t header;
    uint16_t version;
    uint32_t realmode_swtch;
    uint16_t start_sys_seg;
    uint16_t kernel_version;
    uint8_t type_of_loader;
    uint8_t loadflags;
    uint16_t setup_move_size;
    uint32_t code32_start;
    uint32_t ramdisk_image;
    uint32_t ramdisk_size;
    uint32_t bootsect_kludge;
    uint16_t heap_end_ptr;
    uint8_t ext_loader_ver;
    uint8_t ext_loader_type;
    uint32_t cmd_line_ptr;
    uint32_t initrd_addr_max;
    uint32_t kernel_alignment;
    uint8_t relocatable_kernel;
    uint8_t min_alignment;
    uint16_t xloadflags;
    uint32_t cmdline_size;
    uint32_t hardware_subarch;
    uint64_t hardware_subarch_data;
    uint32_t payload_offset;
    uint32_t payload_length;
    uint64_t setup_data;
    uint64_t pref_address;
    uint32_t init_size;
    uint32_t handover_offset;
    uint32_t kernel_info_offset;
} __attribute__((packed));

// [4] The E820 memory region entry of the boot protocol ABI:
struct boot_e820_entry {
    uint64_t addr;
    uint64_t size;
    uint32_t type;
} __attribute__((packed));

typedef struct linux_setup_ret {
    uintptr_t kernel_entry_gpa;
    uintptr_t pml4_gpa;
    uintptr_t zero_page_gpa;
    uintptr_t gdt_gpa;
    uint64_t gdt_limit;
} linux_x86_setup_ret_t;

bool linux_setup_images(uintptr_t ram_start, size_t ram_size, uintptr_t kernel, size_t kernel_size,
                        uintptr_t initrd_src, size_t initrd_size, void *dsdt_blob, uint64_t dsdt_blob_size,
                        char *cmdline, linux_x86_setup_ret_t *ret);