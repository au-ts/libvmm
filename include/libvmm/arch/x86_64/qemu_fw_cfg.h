/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include <libvmm/arch/x86_64/acpi.h>
#include <libvmm/arch/x86_64/e820.h>
#include <libvmm/arch/x86_64/ioports.h>

#define NUM_FW_E820_ENTRIES 1
struct fw_cfg_e820_map {
    struct boot_e820_entry entries[NUM_FW_E820_ENTRIES];
};

#define DSDT_MAX_SIZE 512
struct fw_cfg_acpi_tables {
    // this is the "etc/acpi/tables"
    struct xsdt xsdt;
    struct FADT fadt;
    struct hpet hpet;
    struct madt madt;
    char dsdt[DSDT_MAX_SIZE];
};

// Useful data that will be served to the guest firmware via the fw cfg interface.
struct fw_cfg_blobs {
    struct fw_cfg_e820_map fw_cfg_e820_map;
    struct fw_cfg_acpi_tables fw_acpi_tables;
};

bool emulate_qemu_fw_cfg_access(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read, bool is_string, bool is_rep, ioport_access_width_t access_width);
