/*
 * Copyright 2026, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <libvmm/guest_ram.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/uefi.h>
#include <libvmm/arch/x86_64/e820.h>
#include <libvmm/uefi/fw_cfg.h>
#include <sddf/util/util.h>

/* Document referenced:
 * https://github.com/tianocore/edk2/blob/edk2-stable202605/OvmfPkg/README */

/* Backing storage for the E820 table we need to provide to the firmware, static since
 * the fw_cfg code expects the lifetime of the buffer to be equal to the lifetime of the VMM.
 * Since the guest can access the fw cfg at any time. */
struct boot_e820_entry uefi_e820[GUEST_MAX_RAM_REGIONS];

bool uefi_setup_images(uintptr_t firm_src, size_t firm_size, uint64_t flash_gpa, size_t flash_size)
{
    if (!firm_size) {
        LOG_VMM_ERR("Firmware size is zero\n");
        return false;
    }

    if (!flash_size) {
        LOG_VMM_ERR("Flash size is zero\n");
        return false;
    }

    if (flash_gpa + flash_size != BIT(32)) {
        LOG_VMM_ERR("Flash [0x%lx..0x%lx), size 0x%lu does not reside at top of 4GB in guest RAM.\n", flash_gpa,
                    flash_gpa + flash_size, flash_size);
        return false;
    }

    if (firm_size > flash_size) {
        LOG_VMM_ERR("Firmware size %lu bytes > flash size %lu bytes\n", firm_size, flash_gpa);
        return false;
    }

    void *dest = gpa_to_hva(flash_gpa + (flash_size - firm_size), firm_size);
    if (!dest) {
        LOG_VMM_ERR(
            "Failed to translate GPA to HVA for firmware copy. Did you add the flash as a guest memory region?\n");
        return false;
    }

    /* Places the firmware at the reset vector, see referenced document for more details. */
    memcpy(dest, (void *)firm_src, firm_size);

    /* Initialise the fw cfg device to give the E820 memory map, ACPI tables and other important
     * things to OVMF. */
    if (!initialise_fw_cfg()) {
        return false;
    }

    int num_guest_ram_regions;
    struct guest_ram_region *guest_ram_regions = guest_ram_get_regions(&num_guest_ram_regions);
    assert(num_guest_ram_regions); // impossible, should have been checked earlier

    for (int i = 0; i < num_guest_ram_regions; i++) {
        uefi_e820[i].addr = guest_ram_regions[i].gpa_start;
        uefi_e820[i].size = guest_ram_regions[i].size;
        uefi_e820[i].type = E820_RAM;
    }

    if (!fw_cfg_add_named_file(E820_FWCFG_FILENAME, strlen(E820_FWCFG_FILENAME), (uint8_t *)&uefi_e820,
                               sizeof(struct boot_e820_entry) * num_guest_ram_regions)) {
        LOG_VMM_ERR("Failed to add '%s' file to fw cfg\n", E820_FWCFG_FILENAME);
        return false;
    }

    return true;
}