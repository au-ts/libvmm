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
    /* Place the UEFI firmware at the reset vector.
     * Assumes that (GPA of flash_start) + flash_size == 0x1_0000_0000!!!!!! */
    memcpy((void *)(flash_start + (flash_size - firm_size)), firm, firm_size);

    /* Build the various fw cfg blobs that will be needed by TianoCore OVMF */

    fw_cfg_blobs.fw_cfg_e820_map.entries[0].addr = 0;
    fw_cfg_blobs.fw_cfg_e820_map.entries[0].size = ram_size;
    fw_cfg_blobs.fw_cfg_e820_map.entries[0].type = E820_RAM;

    /* Finish by populating File Dir with everything we built */
    fw_cfg_blobs.fw_cfg_file_dir =
        (struct fw_cfg_file_dir) { .num_files = __builtin_bswap32(NUM_FW_CFG_FILES),
                                   .file_entries[0] = {
                                       .name = E820_FWCFG_FILE,
                                       .size = __builtin_bswap32(sizeof(struct fw_cfg_e820_map)),
                                       .select = __builtin_bswap16(FW_CFG_E820),
                                   } };

    return true;
}