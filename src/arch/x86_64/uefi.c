/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/acpi.h>
#include <libvmm/arch/x86_64/uefi.h>
#include <libvmm/arch/x86_64/qemu_fw_cfg.h>
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

    // Then the ACPI XSDP table
    // The GPA of the XSDT will be filled in by a table loader command later
    xsdp_build(&fw_cfg_blobs.fw_xsdp, 0);

    // Then the rest of the ACPI tables, all GPAs will be zero
    madt_build(&fw_cfg_blobs.fw_acpi_tables.madt);
    hpet_build(&fw_cfg_blobs.fw_acpi_tables.hpet);
    fadt_build(&fw_cfg_blobs.fw_acpi_tables.fadt, 0);
    uint64_t dummy_gpas[XSDT_ENTRIES] = { 0 };
    xsdt_build(&fw_cfg_blobs.fw_acpi_tables.xsdt, dummy_gpas, XSDT_ENTRIES);

    if (dsdt_blob_size > DSDT_MAX_SIZE) {
        LOG_VMM_ERR("DSDT blob size is greater than max size in fw cfg (%d > %d), please increase DSDT_MAX_SIZE\n",
                    dsdt_blob_size, DSDT_MAX_SIZE);
        return false;
    }
    memcpy(&fw_cfg_blobs.fw_acpi_tables.dsdt, dsdt_blob, dsdt_blob_size);

    // Now build the list of table loader commands, which:
    // 1. Load all the tables into memory.
    // 2. Patch all the required GPAs.
    // 3. Recompute all table checksums
    int num_cmd = 0;

    

    // Finish by populating File Dir with everything we built
    fw_cfg_blobs.fw_cfg_file_dir =
        (struct fw_cfg_file_dir) { .num_files = __builtin_bswap32(NUM_FW_CFG_FILES),
                                   .file_entries[0] = {
                                       .name = E820_FWCFG_FILE,
                                       .size = __builtin_bswap32(sizeof(struct fw_cfg_e820_map)),
                                       .select = __builtin_bswap16(FW_CFG_E820),
                                   } };

    return true;
}