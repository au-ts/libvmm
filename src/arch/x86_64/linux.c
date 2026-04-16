
/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string.h>
#include <libvmm/guest.h>
#include <libvmm/guest_ram.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/acpi.h>
#include <libvmm/arch/x86_64/linux.h>
#include <libvmm/arch/x86_64/memory_space.h>
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
#define BZIMAGE_MAGIC 0x53726448

/* [5] */
#define XLF_KERNEL_64 BIT(0) // Kernel have 64-bits entry

#define E820_MAX_ENTRIES_ZEROPAGE 128

/* We do not have an assigned ID in the table of loader IDs, so we do 0xff. */
#define SETUP_HDR_TYPE_OF_LOADER  0xff
#define SETUP_HDR_VGA_MODE_NORMAL 0xffff

#define ZERO_PAGE_ACPI_RSDP_OFFSET    0x70
#define ZERO_PAGE_E820_ENTRIES_OFFSET 0x1e8
#define ZERO_PAGE_E820_TABLE_OFFSET   0x2d0

#define PAGE_SIZE_4K 0x1000
#define PAGE_SIZE_2M 0x200000

/* Modern bzImage prefers to be loaded at this address from [1] "1.10. Loading The Rest of The Kernel"
 * If we don't then the kernel decompressor will just relocate itself. */
#define BZIMAGE_LOAD_GPA 0x100000

#define KERNEL_64_HANDOVER_OFFSET 0x200

/* How much room to leave at the end of the kernel image.
 * The kernel decompressor will crash if this value is too small. */
#define KERNEL_END_PADDING 0x1000000

/* [4] */
#define PTE_PRESENT_BIT BIT(0)
#define PTE_RW_BIT BIT(1)
#define PTE_PS_BIT BIT(7) // 1 if point to a page instead of paging structure
#define PTE_ADDR_MASK (0x000ffffffffff000ull)

// Assume 2MiB large page
static bool map_page(uint64_t paging_objects_start_gpa, void *pml4, uint64_t target_gpa, uint64_t *n_pt_created)
{
    uint64_t unused;

    assert(target_gpa % PAGE_SIZE_2M == 0);

    uint16_t pml4_idx = (target_gpa >> (12 + 9 + 9 + 9)) & 0x1ff;
    uint16_t pdpt_idx = (target_gpa >> (12 + 9 + 9)) & 0x1ff;
    uint16_t pd_idx = (target_gpa >> (12 + 9)) & 0x1ff;

    // Check if PML4 pointer to PDPT is present, create PDPT obj and point to it if not.
    uint64_t *pml4_entries = (uint64_t *)pml4;
    uint64_t pml4_entry = pml4_entries[pml4_idx];
    uint64_t pdpt_gpa;
    if (pml4_entry & PTE_PRESENT_BIT) {
        pdpt_gpa = pml4_entry & PTE_ADDR_MASK;
    } else {
        pdpt_gpa = paging_objects_start_gpa + (*n_pt_created * PAGE_SIZE_4K);
        *n_pt_created = *n_pt_created + 1;

        uint64_t bytes_remaining_for_pdpt;
        void *pdpt = gpa_to_vaddr(pdpt_gpa, &bytes_remaining_for_pdpt);
        if (!pdpt || bytes_remaining_for_pdpt < PAGE_SIZE_4K) {
            LOG_VMM_ERR("map_page(): ran out of memory for PDPT object\n");
            return false;
        }
        memset(pdpt, 0, PAGE_SIZE_4K);

        pml4_entries[pml4_idx] = PTE_PRESENT_BIT | PTE_RW_BIT | (pdpt_gpa & PTE_ADDR_MASK);
    }

    // Same thing, but PDPT -> PT
    void *pdpt = gpa_to_vaddr(pdpt_gpa, &unused);
    uint64_t *pdpt_entries = (uint64_t *)pdpt;
    uint64_t pdpt_entry = pdpt_entries[pdpt_idx];
    uint64_t pd_gpa;
    if (pdpt_entry & PTE_PRESENT_BIT) {
        pd_gpa = pdpt_entry & PTE_ADDR_MASK;
    } else {
        pd_gpa = paging_objects_start_gpa + (*n_pt_created * PAGE_SIZE_4K);
        *n_pt_created = *n_pt_created + 1;

        uint64_t bytes_remaining_for_pd;
        void *pd = gpa_to_vaddr(pd_gpa, &bytes_remaining_for_pd);
        if (!pd || bytes_remaining_for_pd < PAGE_SIZE_4K) {
            LOG_VMM_ERR("map_page(): ran out of memory for PD object\n");
            return false;
        }
        memset(pd, 0, PAGE_SIZE_4K);

        pdpt_entries[pdpt_idx] = PTE_PRESENT_BIT | PTE_RW_BIT | (pd_gpa & PTE_ADDR_MASK);
    }

    // Now PT -> Large page
    void *pd = gpa_to_vaddr(pd_gpa, &unused);
    uint64_t *pd_entries = (uint64_t *)pd;
    uint64_t pd_entry = pd_entries[pd_idx];
    if (pd_entry & PTE_PRESENT_BIT) {
        return false;
    } else {
        pd_entries[pd_idx] = PTE_PRESENT_BIT | PTE_RW_BIT | PTE_PS_BIT | (target_gpa & PTE_ADDR_MASK);
        return true;
    }
}

// Create an initial identity page table, paging objects will grow "upwards" from `paging_objects_start_gpa`
// Returns GPA to the PML4 object.
// The number of bytes used for paging objects will be written to `ret_paging_objs_bytes`
// Not correct for multiple VCPU where each guest needs their own page table.
static uintptr_t build_initial_kernel_page_table(uint64_t paging_objects_start_gpa, uint64_t ident_map_start_gpa,
                                                 uint64_t ident_map_end_gpa, uint64_t *ret_paging_objs_bytes)
{
    /* Create the PML4 object */
    assert(paging_objects_start_gpa % PAGE_SIZE_4K == 0);
    uintptr_t pml4_gpa = paging_objects_start_gpa;
    uint64_t n_pt_created = 1;
    uint64_t bytes_remaining;

    void *pml4 = gpa_to_vaddr(pml4_gpa, &bytes_remaining);
    if (!pml4 || bytes_remaining < PAGE_SIZE_4K) {
        LOG_VMM_ERR("build_initial_kernel_page_table(): ran out of memory for paging objects\n");
        *ret_paging_objs_bytes = 0;
        return 0;
    }

    memset(pml4, 0, PAGE_SIZE_4K);

    int num_init_pages = ((ROUND_UP(ident_map_end_gpa, PAGE_SIZE_2M) - ROUND_DOWN(ident_map_start_gpa, PAGE_SIZE_2M)))
                       / PAGE_SIZE_2M;
    uint64_t current_gpa = ROUND_DOWN(ident_map_start_gpa, PAGE_SIZE_2M);
    for (int i = 0; i < num_init_pages; i += 1) {
        uint64_t base_page_gpa = current_gpa + (i * PAGE_SIZE_2M);
        if (!map_page(paging_objects_start_gpa, pml4, base_page_gpa, &n_pt_created)) {
            *ret_paging_objs_bytes = 0;
            return 0;
        }
    }

    *ret_paging_objs_bytes = n_pt_created * PAGE_SIZE_4K;
    return pml4_gpa;
}

bool linux_setup_images(uintptr_t kernel_src, size_t kernel_size, uintptr_t initrd_src, size_t initrd_size,
                        void *dsdt_blob, uint64_t dsdt_blob_size, char *cmdline, seL4_VCPUContext *initial_regs,
                        linux_x86_setup_ret_t *ret)
{
    /* Load the Linux kernel, initrd and other important blobs into guest RAM, the layout is mostly arbitrarily decided:
     * Kernel code + data | Safety padding | Initrd | Cmdline | Zero page | Init paging objs | GDT | ACPI tables
     */

    int num_guest_ram_regions;
    struct guest_ram_region *guest_ram_regions = guest_ram_get_regions(&num_guest_ram_regions);
    if (!num_guest_ram_regions) {
        LOG_VMM_ERR("linux_setup_images(): no recorded guest RAM region, have you called guest_ram_add_region()?\n");
        return false;
    }

    /* Pick the first RAM region to put everything in, all the other regions are usable for the guest. */
    uint64_t ram_start_gpa = guest_ram_regions[0].gpa_start;
    uint64_t ram_end_gpa = guest_ram_regions[0].gpa_end;
    LOG_VMM("linux_setup_images(): chosen guest RAM region [0x%lx..0x%lx), size %lu bytes\n", ram_start_gpa,
            ram_end_gpa, ram_end_gpa - ram_start_gpa);

    /* See [3] for details of operations done in this function. */
    LOG_VMM("linux_setup_images(): kernel size: %lu MiB (%lu bytes)\n", kernel_size / 1024ull / 1024ull, kernel_size);

    struct setup_header *setup_header_src = (struct setup_header *)((char *)kernel_src + IMAGE_SETUP_HEADER_OFFSET);

    /* Make a copy as we don't want to mutate the original image. */
    struct setup_header setup_header;
    memcpy(&setup_header, setup_header_src, sizeof(struct setup_header));

    if (setup_header.header != BZIMAGE_MAGIC) {
        LOG_VMM_ERR("linux_setup_images(): invalid Linux kernel magic or boot protocol too old (< 2.0): expected magic "
                    "0x%x, got 0x%x\n",
                    BZIMAGE_MAGIC, setup_header.header);
        return false;
    }

    uint16_t protocol_version = setup_header.version;
    uint8_t protocol_version_major = protocol_version >> 8;
    uint8_t protocol_version_minor = protocol_version & 0xff;
    if (protocol_version_major < MINIMUM_BOOT_PROT_MAJOR || protocol_version_minor < MINIMUM_BOOT_PROT_MINOR) {
        LOG_VMM_ERR("linux_setup_images(): boot protocol too old (< %d.%d): got %d.%d\n", MINIMUM_BOOT_PROT_MAJOR,
                    MINIMUM_BOOT_PROT_MINOR, protocol_version_major, protocol_version_minor);
        return false;
    }

    LOG_VMM("linux_setup_images(): Linux boot protocol %d.%d\n", protocol_version_major, protocol_version_minor);

    uint16_t kernel_version_offset = setup_header.kernel_version;
    if (kernel_version_offset != 0) {
        char *kernel_version = (char *)(kernel_src + kernel_version_offset + 0x200);
        LOG_VMM("linux_setup_images(): Linux kernel version string: %s\n", kernel_version);
    }

    /* Figure out where the kernel will live in guest memory, for bzImage, it expects to be loaded at
     * `BZIMAGE_LOAD_GPA` so it is easy. */
    uint64_t kernel_load_gpa = BZIMAGE_LOAD_GPA;
    /* [1] syssize = "The size of the protected-mode code in units of 16-byte paragraphs." */
    uint64_t kernel_load_size = setup_header.syssize * 16; // code size
    uint64_t kernel_end_gpa = kernel_load_gpa + setup_header.init_size
                            + KERNEL_END_PADDING; // code + data size + safety margin
    assert(kernel_load_size < kernel_size);
    if (!(kernel_load_gpa >= ram_start_gpa && kernel_end_gpa <= ram_end_gpa)) {
        LOG_VMM_ERR("linux_setup_images(): the first memory region [0x%lx..0x%lx) does not cover Linux kernel load "
                    "destination [0x%lx..0x%lx)\n",
                    ram_start_gpa, ram_end_gpa, kernel_load_gpa, kernel_end_gpa);
        return false;
    }

    LOG_VMM("linux_setup_images(): kernel GPA 0x%lx, load size %lu, init size %lu bytes\n", kernel_load_gpa,
            kernel_load_size, setup_header.init_size);

    /* See "1.10. Loading The Rest of The Kernel" of [1] */
    uint64_t setup_sects = setup_header.setup_sects;
    if (setup_sects == 0) {
        setup_sects = 4;
    }
    uint64_t setup_size = (setup_sects + 1) * 512;

    /* Copy the kernel into guest RAM */
    uint64_t bytes_remaining_for_kernel;
    void *kernel_load_dest = gpa_to_vaddr_or_crash(kernel_load_gpa, &bytes_remaining_for_kernel);
    assert(bytes_remaining_for_kernel >= kernel_load_size); // already checked above
    assert(kernel_load_dest);
    memcpy(kernel_load_dest, (const void *)(kernel_src + setup_size), kernel_load_size);

    /* Copy the initrd to after the kernel code + data, leaving at least 1 page of room for safety. */
    uint64_t initrd_gpa = ROUND_UP(kernel_end_gpa + PAGE_SIZE_4K, PAGE_SIZE_4K);
    uint64_t initrd_end_gpa = initrd_gpa + initrd_size;
    uint64_t bytes_remaining_for_initrd;
    void *initrd_dest = gpa_to_vaddr(initrd_gpa, &bytes_remaining_for_initrd);
    if (!initrd_dest || bytes_remaining_for_initrd < initrd_size) {
        LOG_VMM_ERR("linux_setup_images(): out of memory for initrd, available %lu < needed %lu\n",
                    bytes_remaining_for_initrd, initrd_size);
        return false;
    }
    LOG_VMM("linux_setup_images(): ramdisk GPA 0x%lx, size %lu bytes\n", initrd_gpa, initrd_size);
    memcpy(initrd_dest, (void *)initrd_src, initrd_size);

    /* Copy the cmdline string to after the initrd. */
    uint64_t cmdline_gpa = ROUND_UP(initrd_end_gpa, PAGE_SIZE_4K);
    uint64_t cmdline_len = strlen(cmdline);
    if (cmdline_len > setup_header.cmdline_size) {
        LOG_VMM_ERR("linux_setup_images(): cmdline length %lu bytes > max %lu bytes\n", cmdline_len,
                    setup_header.cmdline_size);
        return false;
    }
    uint64_t bytes_remaining_for_cmdline;
    void *cmdline_dest = gpa_to_vaddr(cmdline_gpa, &bytes_remaining_for_cmdline);
    if (!cmdline_dest || bytes_remaining_for_cmdline < cmdline_len + 1) {
        LOG_VMM_ERR("linux_setup_images(): out of memory for cmdline, available %lu < needed %lu\n",
                    bytes_remaining_for_cmdline, cmdline_len);
        return false;
    }
    strcpy((char *)(cmdline_dest), cmdline);

    LOG_VMM("linux_setup_images(): Linux command line: '%s'\n", (char *)(cmdline_dest));

    /* Place zero page after cmdline. [1] "10. Zero Page" */
    uint64_t zero_page_gpa = ROUND_UP(cmdline_gpa + cmdline_len + 1, PAGE_SIZE_4K);
    uint64_t zero_page_end_gpa = zero_page_gpa + PAGE_SIZE_4K;
    uint64_t bytes_remaining_for_zero_page;
    void *zero_page_dest = gpa_to_vaddr(zero_page_gpa, &bytes_remaining_for_zero_page);
    if (!zero_page_dest || bytes_remaining_for_zero_page < PAGE_SIZE_4K) {
        LOG_VMM_ERR("linux_setup_images(): out of memory for zero page, available %lu < needed %lu\n",
                    bytes_remaining_for_zero_page, PAGE_SIZE_4K);
        return false;
    }

    LOG_VMM("linux_setup_images(): zero page GPA 0x%lx\n", zero_page_gpa);

    /* Fill in bootloader "obligatory fields" of setup_header.
     * See "1.3. Details of Header Fields" of [1]
     */
    setup_header.vid_mode = SETUP_HDR_VGA_MODE_NORMAL;
    setup_header.type_of_loader = SETUP_HDR_TYPE_OF_LOADER;
    setup_header.ramdisk_image = initrd_gpa;
    setup_header.ramdisk_size = initrd_size;
    setup_header.cmd_line_ptr = cmdline_gpa;

    if (!(setup_header.xloadflags & XLF_KERNEL_64)) {
        LOG_VMM_ERR("linux_setup_images(): Kernel must support the 64-bit entry point flag XLF_KERNEL_64\n");
        return false;
    }

    /* Copy `struct setup_header` into the zero page */
    memset(zero_page_dest, 0, PAGE_SIZE_4K);
    memcpy((void *)(((uintptr_t)zero_page_dest) + IMAGE_SETUP_HEADER_OFFSET), &setup_header,
           sizeof(struct setup_header));

    /* Since we are booting into long mode with paging on, we must set up a minimal page table for Linux to start
     * and read the configuration structures. It must covers the zero page, cmdline, GDT and kernel decompressor working set.
     * Here we just map 1GiB of memory from the kernel start to be safe, since we only need max of 5 paging objects for 2MiB pages. */
    uint64_t ident_map_start_gpa = ROUND_DOWN(kernel_load_gpa, PAGE_SIZE_2M);
    uint64_t ident_map_end_gpa = MIN(ram_end_gpa, ROUND_UP(kernel_load_gpa + 1024 * 1024 * 1024, PAGE_SIZE_2M));
    LOG_VMM("linux_setup_images(): Identity page table coverage GPA: [0x%x..0x%x)\n", ident_map_start_gpa,
            ident_map_end_gpa);

    /* Place the paging objects after the zero page. */
    uint64_t paging_objects_start_gpa = ROUND_UP(zero_page_end_gpa, PAGE_SIZE_4K);
    uint64_t paging_objs_bytes;
    uintptr_t pml4_gpa = build_initial_kernel_page_table(paging_objects_start_gpa, ident_map_start_gpa,
                                                         ident_map_end_gpa, &paging_objs_bytes);
    uint64_t paging_objects_end_gpa = paging_objects_start_gpa + paging_objs_bytes;
    LOG_VMM("linux_setup_images(): Identity paging objects GPA: [0x%lx..0x%lx), %lu paging objects created\n",
            paging_objects_start_gpa, paging_objects_end_gpa, paging_objs_bytes / PAGE_SIZE_4K);

    /* Now build and place the GDT after the paging objects. */
    uint64_t gdt_gpa = ROUND_UP(paging_objects_end_gpa, PAGE_SIZE_4K);
    // uint64_t gdt_end_gpa = gdt_gpa + PAGE_SIZE_4K;
    LOG_VMM("linux_setup_images(): GDT GPA: 0x%x\n", gdt_gpa);
    uint64_t bytes_remaining_for_gdt;
    uint64_t *gdt = gpa_to_vaddr(gdt_gpa, &bytes_remaining_for_gdt);
    if (!gdt || bytes_remaining_for_gdt < PAGE_SIZE_4K) {
        LOG_VMM_ERR("linux_setup_images(): out of memory for GDT, available %lu < needed %lu\n",
                    bytes_remaining_for_zero_page, PAGE_SIZE_4K);
        return false;
    }
    gdt[0] = 0;
    gdt[1] = 0x00AF9A000000FFFFull; // @billn breakdown
    gdt[2] = 0x00CF92000000FFFFull;
    uint64_t gdt_limit = 23; // (8 bytes * 3 entries) - 1 as limit inclusive

    /* Now build and place the ACPI tables after the GDT. */
    uint64_t acpi_start_gpa = ROUND_UP(gdt_gpa + PAGE_SIZE_4K, PAGE_SIZE_4K);
    uint64_t num_bytes_used_for_acpi = 0;
    uint64_t acpi_rsdp_gpa = acpi_build_all(acpi_start_gpa, dsdt_blob, dsdt_blob_size, &num_bytes_used_for_acpi);
    if (!num_bytes_used_for_acpi) {
        LOG_VMM_ERR("linux_setup_images(): could not create ACPI tables\n");
        return false;
    }
    uint64_t acpi_end_gpa = acpi_start_gpa + num_bytes_used_for_acpi;
    LOG_VMM("linux_setup_images(): ACPI RSDP 0x%x, ACPI tables GPA: [0x%x..0x%x)\n", acpi_rsdp_gpa, acpi_start_gpa,
            acpi_end_gpa);

    /* Now fill in important bits in the "zero page": the ACPI RDSP and E820 memory table. */
    uint64_t *acpi_rsdp = (uint64_t *)((uintptr_t)zero_page_dest + ZERO_PAGE_ACPI_RSDP_OFFSET);
    *acpi_rsdp = acpi_rsdp_gpa;

    /* Fill out real RAM, ACPI reclaimable region and initial paging objects range. */
    uint8_t *e820_entries = (uint8_t *)((uintptr_t)zero_page_dest + ZERO_PAGE_E820_ENTRIES_OFFSET);
    *e820_entries = 3;
    struct boot_e820_entry *e820_table = (struct boot_e820_entry *)((uintptr_t)zero_page_dest
                                                                    + ZERO_PAGE_E820_TABLE_OFFSET);
    assert(*e820_entries <= E820_MAX_ENTRIES_ZEROPAGE);
    /* Everything from start of RAM to ACPI tables */
    e820_table[0] = (struct boot_e820_entry) {
        .addr = ram_start_gpa,
        .size = acpi_start_gpa - ram_start_gpa,
        .type = E820_RAM,
    };
    /* ACPI tables */
    e820_table[1] = (struct boot_e820_entry) {
        .addr = acpi_start_gpa,
        .size = num_bytes_used_for_acpi,
        .type = E820_ACPI,
    };
    /* Remaining free RAM */
    e820_table[2] = (struct boot_e820_entry) {
        .addr = acpi_end_gpa,
        .size = ram_end_gpa - acpi_end_gpa,
        .type = E820_RAM,
    };

    /* Linux boot ABI expects physical address of zero page to be in RSI.
     * See [1] "1.15. 64-bit Boot Protocol" */
    memset(initial_regs, 0, sizeof(seL4_VCPUContext));
    initial_regs->esi = zero_page_gpa;

    ret->kernel_entry_gpa = kernel_load_gpa + KERNEL_64_HANDOVER_OFFSET;
    ret->pml4_gpa = pml4_gpa;
    ret->gdt_gpa = gdt_gpa;
    ret->gdt_limit = gdt_limit;

    LOG_VMM("linux_setup_images(): Kernel + Ramdisk + ACPI tables loaded successfully\n");

    return true;
}