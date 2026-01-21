/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/acpi.h>
#include <libvmm/arch/x86_64/uefi.h>
#include <libvmm/arch/x86_64/qemu/fw_cfg.h>
#include <libvmm/arch/x86_64/qemu/ramfb.h>
#include <libvmm/arch/x86_64/e820.h>
#include <sddf/util/custom_libc/string.h>
#include <sddf/util/util.h>

extern struct fw_cfg_blobs fw_cfg_blobs;

bool uefi_setup_images(uintptr_t ram_start, size_t ram_size, uintptr_t flash_start, size_t flash_size, char *firm,
                       size_t firm_size, void *dsdt_blob, uint64_t dsdt_blob_size)
{
    // Place the UEFI firmware at the reset vector.
    // Assumes that (GPA of flash_start) + flash_size == 0x1_0000_0000!!!!!!
    memcpy((void *)(flash_start + (flash_size - firm_size)), firm, firm_size);

    // Build the various fw cfg blobs that will be needed by TianoCore OVMF
    // First thing is the E820 table.
    fw_cfg_blobs.fw_cfg_e820_map.entries[0].addr = 0;
    fw_cfg_blobs.fw_cfg_e820_map.entries[0].size = ram_size;
    fw_cfg_blobs.fw_cfg_e820_map.entries[0].type = E820_RAM;
    LOG_VMM("uefi_setup_images(): guest RAM GPA 0x0..0x%lx\n", ram_size);

    // Then the ACPI XSDP table
    // The GPA of the XSDT will be filled in by a table loader command later
    xsdp_build(&fw_cfg_blobs.fw_xsdp, 0);

    // Then the rest of the ACPI tables, all GPAs will be zero
    madt_build(&fw_cfg_blobs.fw_acpi_tables.madt);
    hpet_build(&fw_cfg_blobs.fw_acpi_tables.hpet);
    fadt_build(&fw_cfg_blobs.fw_acpi_tables.fadt, 0);
    mcfg_build(&fw_cfg_blobs.fw_acpi_tables.mcfg);
    uint64_t dummy_gpas[XSDT_ENTRIES] = { 0 };
    xsdt_build(&fw_cfg_blobs.fw_acpi_tables.xsdt, dummy_gpas, XSDT_ENTRIES);

    if (dsdt_blob_size > DSDT_MAX_SIZE) {
        LOG_VMM_ERR("DSDT blob size is greater than max size in fw cfg (%d > %d), please increase DSDT_MAX_SIZE\n",
                    dsdt_blob_size, DSDT_MAX_SIZE);
        return false;
    }
    memcpy(&fw_cfg_blobs.fw_acpi_tables.dsdt, dsdt_blob, dsdt_blob_size);

    // @billn this is not pretty...

    // Now build the list of table loader commands, which:
    // 1. Load all the tables into memory.
    // 2. Patch all the required GPAs.
    // 3. Recompute all table checksums
    int num_cmd = 0;
    bios_linker_loader_alloc(ACPI_BUILD_RSDP_FILE, 1, true, &fw_cfg_blobs.fw_table_loader[num_cmd]);
    num_cmd += 1;
    bios_linker_loader_alloc(ACPI_BUILD_TABLE_FILE, 1, false, &fw_cfg_blobs.fw_table_loader[num_cmd]);
    num_cmd += 1;

    // Connect the XSDP to the XSDT, then compute XSDP checksum
    bios_linker_loader_add_pointer(ACPI_BUILD_RSDP_FILE, (void *)&fw_cfg_blobs.fw_xsdp, sizeof(struct xsdp),
                                   (uint64_t)&fw_cfg_blobs.fw_xsdp.xsdt_gpa - (uint64_t)&fw_cfg_blobs.fw_xsdp,
                                   sizeof(uint64_t), ACPI_BUILD_TABLE_FILE, 0, &fw_cfg_blobs.fw_table_loader[num_cmd]);
    num_cmd += 1;

    bios_linker_loader_add_checksum(ACPI_BUILD_RSDP_FILE, (void *)&fw_cfg_blobs.fw_xsdp, sizeof(struct xsdp), 0,
                                    offsetof(struct xsdp, length),
                                    (uint64_t)&fw_cfg_blobs.fw_xsdp.checksum - (uint64_t)&fw_cfg_blobs.fw_xsdp,
                                    &fw_cfg_blobs.fw_table_loader[num_cmd]);
    num_cmd += 1;

    bios_linker_loader_add_checksum(ACPI_BUILD_RSDP_FILE, (void *)&fw_cfg_blobs.fw_xsdp, sizeof(struct xsdp), 0,
                                    sizeof(struct xsdp),
                                    (uint64_t)&fw_cfg_blobs.fw_xsdp.ext_checksum - (uint64_t)&fw_cfg_blobs.fw_xsdp,
                                    &fw_cfg_blobs.fw_table_loader[num_cmd]);
    num_cmd += 1;

    // Connect the DSDT to FADT, then checksum the FADT
    bios_linker_loader_add_pointer(
        ACPI_BUILD_TABLE_FILE, (void *)&fw_cfg_blobs.fw_acpi_tables, sizeof(struct fw_cfg_acpi_tables),
        (uint64_t)&fw_cfg_blobs.fw_acpi_tables.fadt.X_Dsdt - (uint64_t)&fw_cfg_blobs.fw_acpi_tables, sizeof(uint64_t),
        ACPI_BUILD_TABLE_FILE, (uint64_t)&fw_cfg_blobs.fw_acpi_tables.dsdt - (uint64_t)&fw_cfg_blobs.fw_acpi_tables,
        &fw_cfg_blobs.fw_table_loader[num_cmd]);
    num_cmd += 1;

    bios_linker_loader_add_checksum(
        ACPI_BUILD_TABLE_FILE, (void *)&fw_cfg_blobs.fw_acpi_tables, sizeof(struct fw_cfg_acpi_tables),
        (uint64_t)&fw_cfg_blobs.fw_acpi_tables.fadt - (uint64_t)&fw_cfg_blobs.fw_acpi_tables, sizeof(struct FADT),
        (uint64_t)&fw_cfg_blobs.fw_acpi_tables.fadt.h.checksum - (uint64_t)&fw_cfg_blobs.fw_acpi_tables,
        &fw_cfg_blobs.fw_table_loader[num_cmd]);
    num_cmd += 1;

    // Connect the FADT, HPET, MADT, MCFG to XSDT, then checksum the XSDT
    bios_linker_loader_add_pointer(
        ACPI_BUILD_TABLE_FILE, (void *)&fw_cfg_blobs.fw_acpi_tables, sizeof(struct fw_cfg_acpi_tables),
        (uint64_t)&fw_cfg_blobs.fw_acpi_tables.xsdt.tables[0] - (uint64_t)&fw_cfg_blobs.fw_acpi_tables,
        sizeof(uint64_t), ACPI_BUILD_TABLE_FILE,
        (uint64_t)&fw_cfg_blobs.fw_acpi_tables.fadt - (uint64_t)&fw_cfg_blobs.fw_acpi_tables,
        &fw_cfg_blobs.fw_table_loader[num_cmd]);
    num_cmd += 1;

    bios_linker_loader_add_pointer(
        ACPI_BUILD_TABLE_FILE, (void *)&fw_cfg_blobs.fw_acpi_tables, sizeof(struct fw_cfg_acpi_tables),
        (uint64_t)&fw_cfg_blobs.fw_acpi_tables.xsdt.tables[1] - (uint64_t)&fw_cfg_blobs.fw_acpi_tables,
        sizeof(uint64_t), ACPI_BUILD_TABLE_FILE,
        (uint64_t)&fw_cfg_blobs.fw_acpi_tables.hpet - (uint64_t)&fw_cfg_blobs.fw_acpi_tables,
        &fw_cfg_blobs.fw_table_loader[num_cmd]);
    num_cmd += 1;

    bios_linker_loader_add_pointer(
        ACPI_BUILD_TABLE_FILE, (void *)&fw_cfg_blobs.fw_acpi_tables, sizeof(struct fw_cfg_acpi_tables),
        (uint64_t)&fw_cfg_blobs.fw_acpi_tables.xsdt.tables[2] - (uint64_t)&fw_cfg_blobs.fw_acpi_tables,
        sizeof(uint64_t), ACPI_BUILD_TABLE_FILE,
        (uint64_t)&fw_cfg_blobs.fw_acpi_tables.madt - (uint64_t)&fw_cfg_blobs.fw_acpi_tables,
        &fw_cfg_blobs.fw_table_loader[num_cmd]);
    num_cmd += 1;

    bios_linker_loader_add_pointer(
        ACPI_BUILD_TABLE_FILE, (void *)&fw_cfg_blobs.fw_acpi_tables, sizeof(struct fw_cfg_acpi_tables),
        (uint64_t)&fw_cfg_blobs.fw_acpi_tables.xsdt.tables[3] - (uint64_t)&fw_cfg_blobs.fw_acpi_tables,
        sizeof(uint64_t), ACPI_BUILD_TABLE_FILE,
        (uint64_t)&fw_cfg_blobs.fw_acpi_tables.mcfg - (uint64_t)&fw_cfg_blobs.fw_acpi_tables,
        &fw_cfg_blobs.fw_table_loader[num_cmd]);
    num_cmd += 1;

    bios_linker_loader_add_checksum(
        ACPI_BUILD_TABLE_FILE, (void *)&fw_cfg_blobs.fw_acpi_tables, sizeof(struct fw_cfg_acpi_tables),
        (uint64_t)&fw_cfg_blobs.fw_acpi_tables.xsdt - (uint64_t)&fw_cfg_blobs.fw_acpi_tables, sizeof(struct xsdt),
        (uint64_t)&fw_cfg_blobs.fw_acpi_tables.xsdt.h.checksum - (uint64_t)&fw_cfg_blobs.fw_acpi_tables,
        &fw_cfg_blobs.fw_table_loader[num_cmd]);
    num_cmd += 1;

    assert(num_cmd < NUM_BIOS_LINKER_LOADER_CMD);

    // Finally populate hardware info, which describe various useful properties of our virtual PCI Host Bridge
    memset(&fw_cfg_blobs.fw_hw_info, 0, sizeof(struct hw_info_pci_host_bridge));
    fw_cfg_blobs.fw_hw_info = (struct hw_info_pci_host_bridge) { .header = { .type = HwInfoTypeHostBridge,
                                                                             .size = sizeof(struct host_bridge_info) },
                                                                 .body = {
                                                                    .Flags.Bits.CombineMemPMem = 1,
                                                                    .Attributes = EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO | EFI_PCI_ATTRIBUTE_ISA_IO | EFI_PCI_ATTRIBUTE_ISA_IO_16,
                                                                    .IoStart = 0,
                                                                    .IoSize = 0x4000,
                                                                     .MemStart = 0xD0000000,
                                                                     .MemSize = 0x200000,
                                                                 } };

    // Finish by populating File Dir with everything we built
    fw_cfg_blobs.fw_cfg_file_dir =
        (struct fw_cfg_file_dir) { .num_files = __builtin_bswap32(NUM_FW_CFG_FILES),
                                   .file_entries = {
                                       {
                                           .name = E820_FWCFG_FILE,
                                           .size = __builtin_bswap32(sizeof(struct fw_cfg_e820_map)),
                                           .select = __builtin_bswap16(FW_CFG_E820),
                                       },
                                       {
                                           .name = FRAMEBFUFER_FWCFG_FILE,
                                           .size = __builtin_bswap32(sizeof(struct QemuRamFBCfg)),
                                           .select = __builtin_bswap16(FW_CFG_FRAMEBUFFER),
                                       },
                                       {
                                           .name = ACPI_BUILD_TABLE_FILE,
                                           .size = __builtin_bswap32(sizeof(struct fw_cfg_acpi_tables)),
                                           .select = __builtin_bswap16(FW_CFG_ACPI_TABLES),
                                       },
                                       {
                                           .name = ACPI_BUILD_RSDP_FILE,
                                           .size = __builtin_bswap32(sizeof(struct xsdp)),
                                           .select = __builtin_bswap16(FW_CFG_ACPI_RSDP),
                                       },
                                       {
                                           .name = ACPI_BUILD_LOADER_FILE,
                                           .size = __builtin_bswap32(sizeof(struct BiosLinkerLoaderEntry) * num_cmd),
                                           .select = __builtin_bswap16(FW_CFG_TABLE_LOADER),
                                       },
                                       {
                                           .name = HW_INFO_FILE,
                                           .size = __builtin_bswap32(sizeof(struct hw_info_pci_host_bridge)),
                                           .select = __builtin_bswap16(FW_CFG_HW_INFO),
                                       },
                                   } };

    return true;
}