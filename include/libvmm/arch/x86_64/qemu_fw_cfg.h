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

/* selector key values for "well-known" fw_cfg entries */
#define FW_CFG_SIGNATURE        0x00
#define FW_CFG_ID               0x01
#define FW_CFG_UUID             0x02
#define FW_CFG_RAM_SIZE         0x03
#define FW_CFG_NOGRAPHIC        0x04
#define FW_CFG_NB_CPUS          0x05
#define FW_CFG_MACHINE_ID       0x06
#define FW_CFG_KERNEL_ADDR      0x07
#define FW_CFG_KERNEL_SIZE      0x08
#define FW_CFG_KERNEL_CMDLINE   0x09
#define FW_CFG_INITRD_ADDR      0x0a
#define FW_CFG_INITRD_SIZE      0x0b
#define FW_CFG_BOOT_DEVICE      0x0c
#define FW_CFG_NUMA             0x0d
#define FW_CFG_BOOT_MENU        0x0e
#define FW_CFG_MAX_CPUS         0x0f
#define FW_CFG_KERNEL_ENTRY     0x10
#define FW_CFG_KERNEL_DATA      0x11
#define FW_CFG_INITRD_DATA      0x12
#define FW_CFG_CMDLINE_ADDR     0x13
#define FW_CFG_CMDLINE_SIZE     0x14
#define FW_CFG_CMDLINE_DATA     0x15
#define FW_CFG_SETUP_ADDR       0x16
#define FW_CFG_SETUP_SIZE       0x17
#define FW_CFG_SETUP_DATA       0x18
#define FW_CFG_FILE_DIR         0x19

#define FW_CFG_E820         0x20
#define FW_CFG_ACPI_TABLES  0x21
#define FW_CFG_ACPI_RSDP    0x22
#define FW_CFG_TABLE_LOADER 0x23

/* an individual file entry, 64 bytes total */
struct FWCfgFile {
    /* size of referenced fw_cfg item, big-endian */
    uint32_t size;
    /* selector key of fw_cfg item, big-endian */
    uint16_t select;
    uint16_t reserved;
    /* fw_cfg item name, NUL-terminated ascii */
    char name[56];
}  __attribute__((packed));

/* Structure of FW_CFG_FILE_DIR */
#define NUM_FW_CFG_FILES 4
struct fw_cfg_file_dir {
    uint32_t num_files; // Big endian!
    struct FWCfgFile file_entries[NUM_FW_CFG_FILES];
};

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
#define E820_FWCFG_FILE "etc/e820"
#define ACPI_BUILD_TABLE_FILE "etc/acpi/tables"
#define ACPI_BUILD_RSDP_FILE "etc/acpi/rsdp"
#define ACPI_BUILD_LOADER_FILE "etc/table-loader"

// Useful data that will be served to the guest firmware via the fw cfg interface.
struct fw_cfg_blobs {
    // File Dir
    struct fw_cfg_file_dir fw_cfg_file_dir;
    // "etc/e820"
    struct fw_cfg_e820_map fw_cfg_e820_map;
    // "etc/acpi/rsdp"
    struct xsdp fw_xsdp;
    // "etc/acpi/tables"
    struct fw_cfg_acpi_tables fw_acpi_tables;
    // "etc/table-loader"
    struct BiosLinkerLoaderEntry fw_table_loader[NUM_BIOS_LINKER_LOADER_CMD];
};

bool emulate_qemu_fw_cfg_access(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read, bool is_string, bool is_rep, ioport_access_width_t access_width);
