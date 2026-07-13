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
#include <sddf/util/util.h>

/* Document referenced:
 * https://github.com/tianocore/edk2/blob/edk2-stable202605/OvmfPkg/README */

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

    return true;
}