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
#include <libvmm/arch/x86_64/qemu/bios_linker_loader.h>

#define NUM_FW_E820_ENTRIES 1
struct fw_cfg_e820_map {
    struct boot_e820_entry entries[NUM_FW_E820_ENTRIES];
};

#define DSDT_MAX_SIZE 512
struct fw_cfg_acpi_tables {
    // very important for XSDT to be first, as we will point to this blob from "etc/acpi/rsdp"
    struct xsdt xsdt;
    struct FADT fadt;
    struct hpet hpet;
    struct madt madt;
    char dsdt[DSDT_MAX_SIZE];
} __attribute__((packed));

#define NUM_BIOS_LINKER_LOADER_CMD 16

// Useful data that will be served to the guest firmware via the fw cfg interface.
struct fw_cfg_blobs {
    // "etc/e820"
    struct fw_cfg_e820_map fw_cfg_e820_map;
    // "etc/acpi/tables"
    struct fw_cfg_acpi_tables fw_acpi_tables;
    // "etc/acpi/rsdp"
    struct xsdp fw_xsdp;
    // "etc/table-loader"
    struct BiosLinkerLoaderEntry fw_table_loader[NUM_BIOS_LINKER_LOADER_CMD];
};

bool emulate_qemu_fw_cfg_access(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read, bool is_string, bool is_rep, ioport_access_width_t access_width);
