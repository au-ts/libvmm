/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * The struct definitions in this file is a reimplementation from an equivalent file in EDK II:
 * https://github.com/tianocore/edk2/blob/3fec6254088b8585e35f660b4fe289b179a15349/OvmfPkg/Library/HardwareInfoLib/HardwareInfoTypesLib.h
 * https://github.com/tianocore/edk2/blob/3fec6254088b8585e35f660b4fe289b179a15349/OvmfPkg/Library/HardwareInfoLib/HardwareInfoPciHostBridgeLib.h
 *
 * Hardware info types' definitions.
 * General hardware info types to parse the binary data
 * Copyright 2021 - 2022 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#pragma once

#include <stdint.h>

enum {
    HW_INFO_TYPE_UNDEFINED = 0,
    HW_INFO_TYPE_HOST_BRIDGE = 1,
    HW_INFO_TYPE_QEMU_UEFI_VARS = 2,
    HW_INFO_TYPE_SVSM_VIRTIO_MMIO = 0x1000,
};

struct hw_info_header {
    uint64_t type;
    uint64_t size;
} __attribute__((packed));

/* Host bridge resources information. */
struct host_bridge_info {
    /* Feature tracking, initially 0 */
    uint64_t version;

    /* Host bridge enabled attributes (EFI_PCI_ATTRIBUTE_*) */
    uint64_t attributes;

    union {
        uint32_t uint32;
        struct {
            uint32_t dma_above_4g : 1;
            uint32_t no_extended_config_space : 1;
            uint32_t combine_mem_pmem : 1;
            uint32_t reserved : 29;
        } bits;
    } flags;

    /* Bus number range */
    uint8_t bus_nr_start;
    uint8_t bus_nr_last;

    uint8_t padding[2];

    /*
     * IO aperture
     */
    uint64_t io_start;
    uint64_t io_size;

    /*
     * 32-bit MMIO aperture
     */
    uint64_t mem_start;
    uint64_t mem_size;

    /*
     * 32-bit prefetchable MMIO aperture
     */
    uint64_t pmem_start;
    uint64_t pmem_size;

    /*
     * 64-bit MMIO aperture
     */
    uint64_t mem_above_4g_start;
    uint64_t mem_above_4g_size;

    /*
     * 64-bit prefetchable MMIO aperture
     */
    uint64_t pmem_above_4g_start;
    uint64_t pmem_above_4g_size;

    /*
     * MMIO accessible PCIe config space (ECAM)
     */
    uint64_t pcie_config_start;
    uint64_t pcie_config_size;
} __attribute__((packed));

struct hw_info_pci_host_bridge {
    struct hw_info_header header;
    struct host_bridge_info body;
} __attribute__((packed));

/*
  https://github.com/tianocore/edk2/blob/3fec6254088b8585e35f660b4fe289b179a15349/MdePkg/Include/Protocol/PciRootBridgeIo.h#L4
  PCI Root Bridge I/O protocol as defined in the UEFI 2.0 specification.

  PCI Root Bridge I/O protocol is used by PCI Bus Driver to perform PCI Memory, PCI I/O,
  and PCI Configuration cycles on a PCI Root Bridge. It also provides services to perform
  different types of bus mastering DMA.

  Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

 */
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