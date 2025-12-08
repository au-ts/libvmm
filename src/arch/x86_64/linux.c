
/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/acpi.h>
#include <libvmm/arch/x86_64/linux.h>
#include <sddf/util/custom_libc/string.h>
#include <sddf/util/util.h>

/* Documents referenced:
 * [1] https://www.kernel.org/doc/html/v6.1/x86/boot.html
 * [2] https://www.kernel.org/doc/html/v6.1/x86/zero-page.html
 * [3] Linux: arch/x86/include/uapi/asm/e820.h
 * [4] https://wiki.osdev.org/Paging
 * [5] Linux: arch/x86/include/uapi/asm/bootparam.h
 */

#define MINIMUM_BOOT_PROT_MAJOR 2
/* Minimum kernel 3.8, as we need:
 * 1. relocatable kernel support, and
 * 2. `xloadflags` field to assert that the kernel supports 64-bit entry
 */
#define MINIMUM_BOOT_PROT_MINOR 12

#define IMAGE_SETUP_HEADER_OFFSET 0x1f1
#define HEADER_MAGIC 0x53726448

/* [5] */
#define XLF_KERNEL_64 BIT(0) // Kernel have 64-bits entry

/* [3] */
#define E820_RAM        1
#define E820_RESERVED   2
#define E820_ACPI       3
#define E820_NVS        4
#define E820_UNUSABLE   5
#define E820_PMEM       7

#define E820_MAX_ENTRIES_ZEROPAGE 128

/* We do not have an assigned ID in the table of loader IDs, so we do 0xff. */
#define SETUP_HDR_TYPE_OF_LOADER  0xff
#define SETUP_HDR_VGA_MODE_NORMAL 0xffff

#define ZERO_PAGE_ACPI_RSDP_OFFSET    0x70
#define ZERO_PAGE_E820_ENTRIES_OFFSET 0x1e8
#define ZERO_PAGE_E820_TABLE_OFFSET   0x2d0

#define PAGE_SIZE_4K 0x1000
#define PAGE_SIZE_2M 0x200000

#define RAM_START_GPA 0x0
#define ZERO_PAGE_GPA 0x0
#define CMDLINE_GPA 0x1000
#define GDT_GPA 0x2000
#define BZIMAGE_LOAD_GPA 0x100000

#define KERNEL_64_HANDOVER_OFFSET 0x200

/* [4] */
#define PTE_PRESENT_BIT BIT(0)
#define PTE_RW_BIT BIT(1)
#define PTE_PS_BIT BIT(7) // 1 if point to a page instead of paging structure
#define PTE_ADDR_MASK (0xfffffffff000ull) // @billn check number of Fs

// Assume 2MiB large page
static bool map_page(uintptr_t pml4_gpa, uintptr_t target_gpa, uintptr_t guest_ram_vaddr, uint64_t *n_pt_created)
{
    assert(target_gpa % PAGE_SIZE_2M == 0);

    uint16_t pml4_idx = (target_gpa >> (12 + 9 + 9 + 9)) & 0x1ff;
    uint16_t pdpt_idx = (target_gpa >> (12 + 9 + 9)) & 0x1ff;
    uint16_t pd_idx = (target_gpa >> (12 + 9)) & 0x1ff;

    // Check if PML4 pointer to PDPT is present, create PDPT obj and point to it if not.
    uint64_t *pml4_entries = (uint64_t *)(guest_ram_vaddr + pml4_gpa);
    uint64_t pml4_entry = pml4_entries[pml4_idx];
    uint64_t pdpt_gpa;
    if (pml4_entry & PTE_PRESENT_BIT) {
        pdpt_gpa = pml4_entry & PTE_ADDR_MASK;
    } else {
        pdpt_gpa = pml4_gpa - (PAGE_SIZE_4K * (*n_pt_created + 1));
        memset((void *)(guest_ram_vaddr + pdpt_gpa), 0, PAGE_SIZE_4K);
        *n_pt_created = *n_pt_created + 1;

        pml4_entries[pml4_idx] = PTE_PRESENT_BIT | PTE_RW_BIT | (pdpt_gpa & PTE_ADDR_MASK);
    }

    // Same thing, but PDPT -> PT
    uint64_t *pdpt_entries = (uint64_t *)(guest_ram_vaddr + pdpt_gpa);
    uint64_t pdpt_entry = pdpt_entries[pdpt_idx];
    uint64_t pd_gpa;
    if (pdpt_entry & PTE_PRESENT_BIT) {
        pd_gpa = pdpt_entry & PTE_ADDR_MASK;
    } else {
        pd_gpa = pml4_gpa - (PAGE_SIZE_4K * (*n_pt_created + 1));
        memset((void *)(guest_ram_vaddr + pd_gpa), 0, PAGE_SIZE_4K);
        *n_pt_created = *n_pt_created + 1;

        pdpt_entries[pdpt_idx] = PTE_PRESENT_BIT | PTE_RW_BIT | (pd_gpa & PTE_ADDR_MASK);
    }

    // Now PT -> Large page
    uint64_t *pd_entries = (uint64_t *)(guest_ram_vaddr + pd_gpa);
    uint64_t pd_entry = pd_entries[pd_idx];
    if (pd_entry & PTE_PRESENT_BIT) {
        return false;
    } else {
        pd_entries[pd_idx] = PTE_PRESENT_BIT | PTE_RW_BIT | PTE_PS_BIT | (target_gpa & PTE_ADDR_MASK);
        return true;
    }
}

// Create an initial identity page table, paging objects will grow "downwards"
// Returns GPA to the PML4 object.
// The contiguous range of memory used by all paging structures are written to pt_objs_start_gpa and pt_objs_end_gpa
// Not correct for SMP where each guest needs their own page table.
static uintptr_t build_initial_kernel_page_table(uintptr_t guest_ram_vaddr, size_t ram_size,
                                                 uint64_t ident_map_start_gpa, uint64_t ident_map_end_gpa,
                                                 uint64_t *pt_objs_start_gpa, uint64_t *pt_objs_end_gpa)
{
    assert(ram_size % PAGE_SIZE_4K == 0);
    uint64_t n_pt_created = 0;
    uintptr_t pml4_gpa = (RAM_START_GPA + ram_size) - PAGE_SIZE_4K;
    n_pt_created += 1;

    memset((void *)(guest_ram_vaddr + pml4_gpa), 0, PAGE_SIZE_4K);

    int num_init_pages = ((ROUND_UP(ident_map_end_gpa, PAGE_SIZE_2M) - ROUND_DOWN(ident_map_start_gpa, PAGE_SIZE_2M)))
                       / PAGE_SIZE_2M;
    uint64_t current_gpa = ROUND_DOWN(ident_map_start_gpa, PAGE_SIZE_2M);
    for (int i = 0; i < num_init_pages; i += 1) {
        uint64_t base_page_gpa = current_gpa + (i * PAGE_SIZE_2M);
        assert(map_page(pml4_gpa, base_page_gpa, guest_ram_vaddr, &n_pt_created));
    }

    *pt_objs_start_gpa = pml4_gpa - ((n_pt_created - 1) * PAGE_SIZE_4K);
    *pt_objs_end_gpa = pml4_gpa + PAGE_SIZE_4K;
    return pml4_gpa;
}

bool linux_setup_images(uintptr_t ram_start, size_t ram_size, uintptr_t kernel, size_t kernel_size,
                        uintptr_t initrd_src, size_t initrd_size, char *cmdline, linux_x86_setup_ret_t *ret)
{
    /* See [3] for details of operations done in this function. */

    // @billn todo check that we got a bzImage and not something else.

    if (ram_size % PAGE_SIZE_4K) {
        LOG_VMM_ERR("expected ram size to be page aligned, but got 0x%x\n", ram_size);
        return false;
    }

    LOG_VMM("Linux kernel size: %dMiB (%ld bytes)\n", kernel_size / 1024 / 1024, kernel_size);

    struct setup_header *setup_header_src = (struct setup_header *)((char *)kernel + IMAGE_SETUP_HEADER_OFFSET);

    /* Make a copy as we don't want to mutate the original image. */
    struct setup_header setup_header;
    memcpy(&setup_header, setup_header_src, sizeof(struct setup_header));

    if (setup_header.header != HEADER_MAGIC) {
        LOG_VMM_ERR("invalid Linux kernel magic or boot protocol too old (< 2.0): expected magic 0x%x, got 0x%x\n",
                    HEADER_MAGIC, setup_header.header);
        return false;
    }

    uint16_t protocol_version = setup_header.version;
    uint8_t protocol_version_major = protocol_version >> 8;
    uint8_t protocol_version_minor = protocol_version & 0xff;
    if (protocol_version_major < MINIMUM_BOOT_PROT_MAJOR || protocol_version_minor < MINIMUM_BOOT_PROT_MINOR) {
        LOG_VMM_ERR("boot protocol too old (< %d.%d): got %d.%d\n", MINIMUM_BOOT_PROT_MAJOR, MINIMUM_BOOT_PROT_MINOR,
                    protocol_version_major, protocol_version_minor);
        return false;
    }

    LOG_VMM("Linux boot protocol %d.%d\n", protocol_version_major, protocol_version_minor);

    // @billn hack, assumes guest ram starts from 0
    uint64_t initrd_gpa = ram_size - 0x2000000;
    LOG_VMM("Ramdisk GPA 0x%x, size 0x%x\n", initrd_gpa, initrd_size);
    memcpy((void *)ram_start + initrd_gpa, (void *)initrd_src, initrd_size);

    uint16_t kernel_version_offset = setup_header.kernel_version;
    if (kernel_version_offset != 0) {
        char *kernel_version = (char *)(kernel + kernel_version_offset + 0x200);
        LOG_VMM("Linux kernel version string: %s\n", kernel_version);
    }

    /* Fill in bootloader "obligatory fields" then copy it into guest RAM.
     * See "1.3. Details of Header Fields" of [1]
     */
    setup_header.vid_mode = SETUP_HDR_VGA_MODE_NORMAL;
    setup_header.type_of_loader = SETUP_HDR_TYPE_OF_LOADER;
    setup_header.ramdisk_image = initrd_gpa;
    setup_header.ramdisk_size = initrd_size; // @billn for now
    setup_header.cmd_line_ptr = CMDLINE_GPA;
    assert(strlen(cmdline) <= setup_header.cmdline_size);
    strcpy((char *)(ram_start + CMDLINE_GPA), cmdline);
    LOG_VMM("Linux command line: '%s'\n", (char *)(ram_start + CMDLINE_GPA));

    if (!(setup_header.xloadflags & XLF_KERNEL_64)) {
        LOG_VMM_ERR("Kernel must support the 64-bit entry point flag XLF_KERNEL_64\n");
        return false;
    }

    uintptr_t kernel_load_gpa = BZIMAGE_LOAD_GPA;
    /* See "1.10. Loading The Rest of The Kernel" of [1] */
    uint64_t setup_sects = setup_header.setup_sects;
    if (setup_sects == 0) {
        setup_sects = 4;
    }
    uint64_t setup_size = (setup_sects + 1) * 512;

    LOG_VMM("Linux load GPA 0x%x\n", kernel_load_gpa);

    /* Copy over the important things: kernel code and the "zero page" which contains `struct setup_header` */
    memcpy((void *)ram_start + kernel_load_gpa, (const void *)(kernel + setup_size), setup_header.syssize * 16);
    memset((void *)ram_start + ZERO_PAGE_GPA, 0, PAGE_SIZE_4K);
    memcpy((void *)ram_start + ZERO_PAGE_GPA + IMAGE_SETUP_HEADER_OFFSET, &setup_header, sizeof(struct setup_header));

    /* Since we are booting into long mode, we must set up a minimal page table for Linux to start
     * and read the configuration structures. It must covers the zero page, cmdline, GDT and kernel decompressor working set. */
    uint64_t kernel_entry_gpa = BZIMAGE_LOAD_GPA + KERNEL_64_HANDOVER_OFFSET;
    uint64_t ident_map_start_gpa = MIN(ZERO_PAGE_GPA, kernel_entry_gpa);
    uint64_t ident_map_end_gpa = ROUND_UP(kernel_entry_gpa + setup_header.init_size, PAGE_SIZE_2M);
    /* Map in a bunch more page to prevent the decompressor from crashing, won't cost more memory as we only need 3 paging
     * structures to map 1GiB worth of memory. */
    ident_map_end_gpa += PAGE_SIZE_2M * 50;

    LOG_VMM("Identity page table coverage GPA: [0x%x..0x%x)\n", ident_map_start_gpa, ident_map_end_gpa);
    uint64_t pt_objs_start_gpa, pt_objs_end_gpa;
    uintptr_t pml4_gpa = build_initial_kernel_page_table(ram_start, ram_size, ident_map_start_gpa, ident_map_end_gpa,
                                                         &pt_objs_start_gpa, &pt_objs_end_gpa);
    LOG_VMM("Identity paging objects GPA: [0x%x..0x%x)\n", pt_objs_start_gpa, pt_objs_end_gpa);

    /* Now create the ACPI tables and get the RSDP. */
    uint64_t acpi_start_gpa, acpi_end_gpa;
    /* Pass pt_objs_start_gpa as "ram_top" so that the ACPI tables live straight below the initial paging objects. */
    uint64_t acpi_rsdp_gpa = acpi_rsdp_init(ram_start, pt_objs_start_gpa, &acpi_start_gpa, &acpi_end_gpa);
    LOG_VMM("ACPI RSDP 0x%x, ACPI tables GPA: [0x%x..0x%x)\n", acpi_rsdp_gpa, acpi_start_gpa, acpi_end_gpa);

    /* Now fill in important bits in the "zero page": the ACPI RDSP and E820 memory table. */
    uint64_t *acpi_rsdp = (uint64_t *)(ram_start + ZERO_PAGE_GPA + ZERO_PAGE_ACPI_RSDP_OFFSET);
    *acpi_rsdp = acpi_rsdp_gpa;

    /* Fill out real RAM, ACPI reclaimable region and initial paging objects range. */
    uint8_t *e820_entries = (uint8_t *)(ram_start + ZERO_PAGE_GPA + ZERO_PAGE_E820_ENTRIES_OFFSET);
    *e820_entries = 3;
    struct boot_e820_entry *e820_table = (struct boot_e820_entry *)(ram_start + ZERO_PAGE_GPA
                                                                    + ZERO_PAGE_E820_TABLE_OFFSET);
    assert(*e820_entries <= E820_MAX_ENTRIES_ZEROPAGE);
    e820_table[0] = (struct boot_e820_entry) {
        .addr = pt_objs_start_gpa,
        .size = pt_objs_end_gpa - pt_objs_start_gpa,
        .type = E820_RESERVED,
    };
    e820_table[1] = (struct boot_e820_entry) {
        .addr = acpi_start_gpa,
        .size = acpi_end_gpa - acpi_start_gpa,
        .type = E820_ACPI,
    };
    e820_table[2] = (struct boot_e820_entry) {
        .addr = 0,
        .size = acpi_start_gpa,
        .type = E820_RAM,
    };

    /* Build GDT */
    uint64_t *gdt = (uint64_t *)(ram_start + GDT_GPA);
    gdt[0] = 0;
    gdt[1] = 0x00AF9A000000FFFFull; // @billn breakdown
    gdt[2] = 0x00AF92000000FFFFull;

    ret->kernel_entry_gpa = kernel_entry_gpa;
    ret->pml4_gpa = pml4_gpa;
    ret->zero_page_gpa = ZERO_PAGE_GPA;
    ret->gdt_gpa = GDT_GPA;
    ret->gdt_limit = 24; // 8 * 3 entries

    return true;
}