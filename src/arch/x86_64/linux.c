/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/linux.h>
#include <sddf/util/custom_libc/string.h>
#include <sddf/util/util.h>

/* Document referenced:
 * [1] https://www.kernel.org/doc/html/latest/arch/x86/boot.html
 * [2] https://www.kernel.org/doc/html/latest/arch/x86/zero-page.html
 */

#define IMAGE_SETUP_HEADER_OFFSET 0x1f1
#define HEADER_MAGIC 0x53726448

#define E820_RAM 1

/* We do not have an assigned ID in the table of loader IDs, so we do 0xff. */
#define SETUP_HDR_TYPE_OF_LOADER  0xff
#define SETUP_HDR_VGA_MODE_NORMAL 0xffff

#define ZERO_PAGE_E820_ENTRIES_OFFSET 0x1e8
#define ZERO_PAGE_E820_ENTRIES_TYPE   uint8_t

#define ZERO_PAGE_E820_TABLE_OFFSET   0x2d0

#define PAGE_SIZE_4K 0x1000

#define RAM_START_GPA 0x0
#define ZERO_PAGE_GPA 0x0 // Doesn't have to be at 0, but just matching the name
#define CMDLINE_GPA 0x1000
#define GDT_GPA 0x2000
#define INIT_RSP_GPA 0x10000
#define DEFAULT_KERNEL_GPA 0x20000

#define KERNEL_64_HANDOVER_OFFSET 0x200

// Returns GPA to the PML4 object
static uintptr_t build_initial_kernel_page_table(uintptr_t ram_start, size_t ram_size, uintptr_t kernel_gpa, size_t init_size)
{
    // hackily build an initial page table for [0x0..0x200000)
    uintptr_t ram_size_round_down = ROUND_DOWN(ram_size, PAGE_SIZE_4K) - PAGE_SIZE_4K;
    uintptr_t pml4_gpa = ram_size_round_down - PAGE_SIZE_4K;
    uintptr_t pdpt_gpa = pml4_gpa - PAGE_SIZE_4K;
    uintptr_t pd_gpa = pdpt_gpa - PAGE_SIZE_4K;

    uint64_t *pml4_entries = (uint64_t *) (ram_start + pml4_gpa);
    uint64_t *pdpt_entries = (uint64_t *) (ram_start + pdpt_gpa);
    uint64_t *pd_entries = (uint64_t *) (ram_start + pd_gpa);

    pml4_entries[0] = 1 | 1 << 1 | 1 << 2;
    pdpt_entries[0] = 1 | 1 << 1 | 1 << 2;
    pd_entries[0] = 1 | 1 << 1 | 1 << 2 | 1 << 7;

    return pml4_gpa;
}

bool linux_setup_images(uintptr_t ram_start, size_t ram_size, uintptr_t kernel, size_t kernel_size,
                        uintptr_t initrd_src, size_t initrd_size, char *cmdline, linux_x86_setup_ret_t *ret)
{

    if (ram_size % PAGE_SIZE_4K) {
        LOG_VMM_ERR("expected ram size to be page aligned, but got 0x%x\n", ram_size);
        return false;
    }

    struct setup_header *setup_header_src = (struct setup_header *)((char *)kernel + IMAGE_SETUP_HEADER_OFFSET);

    /* Make a copy as we don't want to mutate the original image. */
    struct setup_header setup_header;
    memcpy(&setup_header, setup_header_src, sizeof(struct setup_header));

    if (setup_header.header != HEADER_MAGIC) {
        LOG_VMM_ERR("invalid Linux kernel magic or boot protocol too old (< 2.0): expected 0x%x, got 0x%x\n",
                    HEADER_MAGIC, setup_header.header);
        return false;
    }

    uint16_t protocol_version = setup_header.version;
    uint8_t protocol_version_major = protocol_version >> 8;
    uint8_t protocol_version_minor = protocol_version & 0xff;
    LOG_VMM("Linux boot protocol %d.%d\n", protocol_version_major, protocol_version_minor);

    uint16_t kernel_version_offset = setup_header.kernel_version;
    if (kernel_version_offset != 0) {
        char *kernel_version = (char *)(kernel + kernel_version_offset + 0x200);
        LOG_VMM("Linux kernel version string: %s\n", kernel_version);
    }

    /* Fill in bootloader "obligatory fields" then copy it into guest RAM. */
    setup_header.vid_mode = SETUP_HDR_VGA_MODE_NORMAL;
    setup_header.type_of_loader = SETUP_HDR_TYPE_OF_LOADER;
    setup_header.ramdisk_image = 0; // @billn for now
    setup_header.ramdisk_size = 0; // @billn for now
    setup_header.cmd_line_ptr = CMDLINE_GPA;
    strcpy((char *)(ram_start + CMDLINE_GPA), cmdline);
    LOG_VMM("Linux command line: '%s'\n", cmdline);

    if (!(setup_header.xloadflags & 1)) {
        LOG_VMM_ERR("Kernel must support the 64-bit entry point flag XLF_KERNEL_64");
        return false;
    }

    /* Check where Linux wants to be loaded. But not listen to it verbatim as we are reserving memory for
     * configuration structures and setting up the kernel's page table later. */
    uintptr_t kernel_gpa = DEFAULT_KERNEL_GPA;
    if (setup_header.pref_address) {
        LOG_VMM("Linux load GPA 0x%x\n", setup_header.pref_address);
        // kernel_gpa = setup_header.pref_address; // @billn revisit
    }
    // @billn check minimum alignment of DEFAULT_KERNEL_GPA
    // @billn check that preferred address doesnt overlap with things

    /* Copy over the important things: kernel and the "zero page" which contains `struct setup_header` */
    memcpy((void *)ram_start + kernel_gpa, (const void *)kernel, kernel_size);
    memset((void *)ram_start + ZERO_PAGE_GPA, 0, PAGE_SIZE_4K);
    memcpy((void *)ram_start + ZERO_PAGE_GPA + IMAGE_SETUP_HEADER_OFFSET, (const void *)&setup_header,
           sizeof(struct setup_header));

    /* Now fill in important bits in the "zero page": the ACPI RDSP and E820 memory table. */
    // @billn acpi rsdp

    /* Just 1 memory region for now */
    uint8_t *e820_entries = (uint8_t *)(ram_start + ZERO_PAGE_GPA + ZERO_PAGE_E820_ENTRIES_OFFSET);
    *e820_entries = 1;
    struct boot_e820_entry *e820_table = (struct boot_e820_entry *)(ram_start + ZERO_PAGE_GPA
                                                                    + ZERO_PAGE_E820_TABLE_OFFSET);
    e820_table[0].addr = RAM_START_GPA;
    e820_table[1].size = ram_size;
    e820_table[2].type = E820_RAM;

    /* Since we are booting into long mode, we must set up a minimal page table for Linux to start
     * and read the configuration structures. */
    LOG_VMM("Linux init region: [0x%x..0x%x)\n", kernel_gpa, kernel_gpa + setup_header.init_size);

    /* First make the PML4 (top level) paging object. Place it at the end of memory to not conflict with
     * anything vital. */
    uintptr_t pml4_gpa = build_initial_kernel_page_table(ram_start, ram_size, kernel_gpa, setup_header.init_size);

    /* Build GDT */
    uint64_t *gdt = (uint16_t *) (ram_start + GDT_GPA);
    gdt[0] = 0;
    gdt[1] = 0xFFFF | (1 << 49) | (1 << 47) | 0xf << 48;
    gdt[2] = 0xFFFF | 0xf << 48 | (1 << 41) | (1 << 47);

    ret->kernel_entry_gpa = kernel_gpa + KERNEL_64_HANDOVER_OFFSET;
    ret->kernel_stack_gpa = INIT_RSP_GPA;
    ret->pml4_gpa = pml4_gpa;
    ret->zero_page_gpa = ZERO_PAGE_GPA;

    return true;
}