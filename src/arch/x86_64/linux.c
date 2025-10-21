/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <libvmm/guest.h>
#include <libvmm/util/util.h>
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
#define E820_RAM 1

/* We do not have an assigned ID in the table of loader IDs, so we do 0xff. */
#define SETUP_HDR_TYPE_OF_LOADER  0xff
#define SETUP_HDR_VGA_MODE_NORMAL 0xffff

#define ZERO_PAGE_ACPI_RSDP_OFFSET    0x70
#define ZERO_PAGE_E820_ENTRIES_OFFSET 0x1e8
#define ZERO_PAGE_E820_TABLE_OFFSET   0x2d0

#define PAGE_SIZE_4K 0x1000
#define PAGE_SIZE_2M 0x200000

#define RAM_START_GPA 0x0
// @billn, Linux boot protocol expects zero page GPA to be in %rsi at entry,
// but there is no way to set this in seL4, so leaving it as zero since all
// registers are conveniently zero on entry, revisit.
#define ZERO_PAGE_GPA 0x0
#define CMDLINE_GPA 0x1000
#define GDT_GPA 0x2000
#define DEFAULT_KERNEL_LOAD_GPA 0x100000

#define KERNEL_64_HANDOVER_OFFSET 0x200

/* [4] */
#define PTE_PRESENT_BIT BIT(0)
#define PTE_RW_BIT BIT(1)
#define PTE_PS_BIT BIT(7) // 1 if point to a page instead of paging structure
#define PTE_ADDR_MASK (0xfffffffff000ull) // @billn check number of Fs

// Assume 2MiB large page
static bool map_page(uintptr_t pml4_gpa, uintptr_t target_gpa, uintptr_t ram_start, uint64_t ram_size_round_down,
                     uint64_t *n_pt_created)
{
    assert(target_gpa % PAGE_SIZE_2M == 0);

    uint16_t pml4_idx = (target_gpa >> (12 + 9 + 9 + 9)) & 0x1ff;
    uint16_t pdpt_idx = (target_gpa >> (12 + 9 + 9)) & 0x1ff;
    uint16_t pd_idx = (target_gpa >> (12 + 9)) & 0x1ff;

    // Check if PML4 pointer to PDPT is present, create PDPT obj and point to it if not.
    uint64_t *pml4_entries = (uint64_t *)(ram_start + pml4_gpa);
    uint64_t pml4_entry = pml4_entries[pml4_idx];
    uint64_t pdpt_gpa;
    if (pml4_entry & PTE_PRESENT_BIT) {
        pdpt_gpa = pml4_entry & PTE_ADDR_MASK;
    } else {
        pdpt_gpa = ram_size_round_down - (PAGE_SIZE_4K * (*n_pt_created + 1));
        *n_pt_created = *n_pt_created + 1;

        pml4_entries[pml4_idx] = PTE_PRESENT_BIT | PTE_RW_BIT | (pdpt_gpa & PTE_ADDR_MASK);
    }

    // Same thing, but PDPT -> PT
    uint64_t *pdpt_entries = (uint64_t *)(ram_start + pdpt_gpa);
    uint64_t pdpt_entry = pdpt_entries[pdpt_idx];
    uint64_t pd_gpa;
    if (pdpt_entry & PTE_PRESENT_BIT) {
        pd_gpa = pdpt_entry & PTE_ADDR_MASK;
    } else {
        pd_gpa = ram_size_round_down - (PAGE_SIZE_4K * (*n_pt_created + 1));
        *n_pt_created = *n_pt_created + 1;

        pdpt_entries[pdpt_idx] = PTE_PRESENT_BIT | PTE_RW_BIT | (pd_gpa & PTE_ADDR_MASK);
    }

    // Now PT -> Large page
    uint64_t *pd_entries = (uint64_t *)(ram_start + pd_gpa);
    uint64_t pd_entry = pd_entries[pd_idx];
    if (pd_entry & PTE_PRESENT_BIT) {
        // @billn print error
        return false;
    } else {
        pd_entries[pd_idx] = PTE_PRESENT_BIT | PTE_RW_BIT | PTE_PS_BIT | (target_gpa & PTE_ADDR_MASK);
        return true;
    }
}

// Returns GPA to the PML4 object
// Not correct for SMP where each guest needs their own page table.
static uintptr_t build_initial_kernel_page_table(uintptr_t ram_start, size_t ram_size, uintptr_t kernel_entry_gpa,
                                                 size_t init_size)
{
    uint64_t n_pt_created = 0;
    uint64_t ram_size_round_down = ROUND_DOWN(ram_size, PAGE_SIZE_4K);
    uintptr_t pml4_gpa = ram_size_round_down - (PAGE_SIZE_4K * (n_pt_created + 1));
    n_pt_created += 1;

    memset((void *)(ram_start + pml4_gpa), 0, PAGE_SIZE_4K);

    // Map range [0x0..0x200000) for zero page, cmdline and GDT
    assert(map_page(pml4_gpa, 0, ram_start, ram_size_round_down, &n_pt_created));

    // Then kernel entry..kernel entry + init_size
    // todo check no overlap
    int num_init_pages =
        ((ROUND_UP(kernel_entry_gpa + init_size, PAGE_SIZE_2M) - ROUND_DOWN(kernel_entry_gpa, PAGE_SIZE_2M))
         / PAGE_SIZE_2M)
        + 1;
    for (int i = 0; i < num_init_pages; i += 1) {
        uint64_t base_page_gpa = ROUND_DOWN(kernel_entry_gpa + (i * PAGE_SIZE_2M), PAGE_SIZE_2M);
        assert(map_page(pml4_gpa, base_page_gpa, ram_start, ram_size_round_down, &n_pt_created));
    }

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

    LOG_VMM("Linux kernel size: %ld bytes\n", kernel_size);

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
    setup_header.ramdisk_image = 0; // @billn for now
    setup_header.ramdisk_size = 0; // @billn for now
    setup_header.cmd_line_ptr = CMDLINE_GPA;
    strcpy((char *)(ram_start + CMDLINE_GPA), cmdline);
    LOG_VMM("Linux command line: '%s'\n", cmdline);

    if (!(setup_header.xloadflags & XLF_KERNEL_64)) {
        LOG_VMM_ERR("Kernel must support the 64-bit entry point flag XLF_KERNEL_64\n");
        return false;
    }

    /* Check where Linux wants to be loaded. But not listen to it verbatim as we are reserving memory for
     * configuration structures and setting up the kernel's page table later. */
    uintptr_t kernel_load_gpa = DEFAULT_KERNEL_LOAD_GPA;
    if (setup_header.pref_address) {
        LOG_VMM("Linux preferred load GPA 0x%x\n", setup_header.pref_address);
        kernel_load_gpa = setup_header.pref_address;
        // @billn todo check that this doesnt overlap with anything important
    }

    /* Copy over the important things: bzImage and the "zero page" which contains `struct setup_header` */
    memcpy((void *)ram_start + kernel_load_gpa, (const void *)(kernel), kernel_size);
    memset((void *)ram_start + ZERO_PAGE_GPA, 0, PAGE_SIZE_4K);
    memcpy((void *)ram_start + ZERO_PAGE_GPA + IMAGE_SETUP_HEADER_OFFSET, (const void *)&setup_header,
           sizeof(struct setup_header));

    /* Now fill in important bits in the "zero page": the ACPI RDSP and E820 memory table. */
    // @billn acpi rsdp
    uint64_t *acpi_rsdp = (uint64_t *)(ram_start + ZERO_PAGE_GPA + ZERO_PAGE_ACPI_RSDP_OFFSET);
    *acpi_rsdp = 0x88abcdef;

    /* Just 1 memory region for now */
    uint8_t *e820_entries = (uint8_t *)(ram_start + ZERO_PAGE_GPA + ZERO_PAGE_E820_ENTRIES_OFFSET);
    *e820_entries = 1;
    struct boot_e820_entry *e820_table = (struct boot_e820_entry *)(ram_start + ZERO_PAGE_GPA
                                                                    + ZERO_PAGE_E820_TABLE_OFFSET);
    e820_table[0].addr = RAM_START_GPA;
    e820_table[0].size = ram_size;
    e820_table[0].type = E820_RAM;

    /* See "1.10. Loading The Rest of The Kernel" of [1] */
    uint64_t setup_sects = setup_header.setup_sects;
    if (setup_sects == 0) {
        setup_sects = 4;
    }
    uint64_t kernel_entry_gpa = kernel_load_gpa + ((setup_sects + 1) * 512) + KERNEL_64_HANDOVER_OFFSET;

    /* Since we are booting into long mode, we must set up a minimal page table for Linux to start
     * and read the configuration structures. */
    LOG_VMM("Linux init region: [0x%x..0x%x)\n", kernel_entry_gpa, kernel_entry_gpa + setup_header.init_size);
    uintptr_t pml4_gpa = build_initial_kernel_page_table(ram_start, ram_size, kernel_entry_gpa, setup_header.init_size);

    /* Build GDT */
    uint64_t *gdt = (uint64_t *)(ram_start + GDT_GPA);
    gdt[0] = 0;
    gdt[1] = 0x00AF9A000000FFFFull; // @billn breakdown
    gdt[2] = 0x00AF92000000FFFFull;

    ret->kernel_entry_gpa = kernel_entry_gpa;
    ret->pml4_gpa = pml4_gpa;
    ret->zero_page_gpa = ZERO_PAGE_GPA;
    ret->gdt_gpa = GDT_GPA;

    return true;
}