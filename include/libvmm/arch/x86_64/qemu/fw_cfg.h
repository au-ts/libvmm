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
#define FW_CFG_FRAMEBUFFER  0x24
#define FW_CFG_HW_INFO      0x25

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
#define NUM_FW_CFG_FILES 10
struct fw_cfg_file_dir {
    uint32_t num_files; // Big endian!
    struct FWCfgFile file_entries[NUM_FW_CFG_FILES];
} __attribute__((packed));

#define NUM_FW_E820_ENTRIES 2
struct fw_cfg_e820_map {
    struct boot_e820_entry entries[NUM_FW_E820_ENTRIES];
} __attribute__((packed));

#define DSDT_MAX_SIZE 512
struct fw_cfg_acpi_tables {
    // very important for XSDT to be first, as we will point to this blob from "etc/acpi/rsdp"
    struct xsdt xsdt;
    struct FADT fadt;
    struct hpet hpet;
    struct madt madt;
    struct mcfg mcfg;
    char dsdt[DSDT_MAX_SIZE];
} __attribute__((packed));

/**@file
  Hardware info library with types and accessors to parse information about
  PCI host bridges.

  Copyright 2021 - 2022 Amazon.com, Inc. or its affiliates. All Rights Reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
// OvmfPkg/Library/HardwareInfoLib/HardwareInfoTypesLib.h
enum {
    HwInfoTypeUndefined = 0,
    HwInfoTypeHostBridge = 1,
    HwInfoTypeQemuUefiVars = 2,
    HwInfoTypeSvsmVirtioMmio = 0x1000,
};

struct hw_info_header {
    uint64_t type;
    uint64_t size;
} __attribute__((packed));

// OvmfPkg/Library/HardwareInfoLib/HardwareInfoPciHostBridgeLib.h
struct host_bridge_info {
  // Feature tracking, initially 0
  uint64_t    Version;

  // Host bridge enabled attributes (EFI_PCI_ATTRIBUTE_*)
  uint64_t    Attributes;

  union {
    uint32_t    Uint32;
    struct {
      uint32_t    DmaAbove4G            : 1;
      uint32_t    NoExtendedConfigSpace : 1;
      uint32_t    CombineMemPMem        : 1;
      uint32_t    Reserved              : 29;
    } Bits;
  } Flags;

  // Bus number range
  uint8_t     BusNrStart;
  uint8_t     BusNrLast;

  uint8_t     Padding[2];

  //
  // IO aperture
  //
  uint64_t    IoStart;
  uint64_t    IoSize;

  //
  // 32-bit MMIO aperture
  //
  uint64_t    MemStart;
  uint64_t    MemSize;

  //
  // 32-bit prefetchable MMIO aperture
  //
  uint64_t    PMemStart;
  uint64_t    PMemSize;

  //
  // 64-bit MMIO aperture
  //
  uint64_t    MemAbove4GStart;
  uint64_t    MemAbove4GSize;

  //
  // 64-bit prefetchable MMIO aperture
  //
  uint64_t    PMemAbove4GStart;
  uint64_t    PMemAbove4GSize;

  //
  // MMIO accessible PCIe config space (ECAM)
  //
  uint64_t    PcieConfigStart;
  uint64_t    PcieConfigSize;
} __attribute__((packed));

struct hw_info_pci_host_bridge {
    struct hw_info_header header;
    struct host_bridge_info body;
};

// MdePkg/Include/Protocol/PciRootBridgeIo.h
/** @file
  PCI Root Bridge I/O protocol as defined in the UEFI 2.0 specification.

  PCI Root Bridge I/O protocol is used by PCI Bus Driver to perform PCI Memory, PCI I/O,
  and PCI Configuration cycles on a PCI Root Bridge. It also provides services to perform
  defferent types of bus mastering DMA.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#define EFI_PCI_ATTRIBUTE_ISA_MOTHERBOARD_IO    0x0001
#define EFI_PCI_ATTRIBUTE_ISA_IO                0x0002
#define EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO        0x0004
#define EFI_PCI_ATTRIBUTE_VGA_MEMORY            0x0008
#define EFI_PCI_ATTRIBUTE_VGA_IO                0x0010
#define EFI_PCI_ATTRIBUTE_IDE_PRIMARY_IO        0x0020
#define EFI_PCI_ATTRIBUTE_IDE_SECONDARY_IO      0x0040
#define EFI_PCI_ATTRIBUTE_MEMORY_WRITE_COMBINE  0x0080
#define EFI_PCI_ATTRIBUTE_MEMORY_CACHED         0x0800
#define EFI_PCI_ATTRIBUTE_MEMORY_DISABLE        0x1000
#define EFI_PCI_ATTRIBUTE_DUAL_ADDRESS_CYCLE    0x8000
#define EFI_PCI_ATTRIBUTE_ISA_IO_16             0x10000
#define EFI_PCI_ATTRIBUTE_VGA_PALETTE_IO_16     0x20000
#define EFI_PCI_ATTRIBUTE_VGA_IO_16             0x40000

#define NUM_BIOS_LINKER_LOADER_CMD 16
#define E820_FWCFG_FILE "etc/e820"
#define ACPI_BUILD_TABLE_FILE "etc/acpi/tables"
#define ACPI_BUILD_RSDP_FILE "etc/acpi/rsdp"
#define ACPI_BUILD_LOADER_FILE "etc/table-loader"
#define FRAMEBFUFER_FWCFG_FILE "etc/ramfb"
#define HW_INFO_FILE "etc/hardware-info"

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
    // "etc/hardware-info"
    struct hw_info_pci_host_bridge fw_hw_info;
};

bool emulate_qemu_fw_cfg_access(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read, bool is_string, bool is_rep, ioport_access_width_t access_width);
