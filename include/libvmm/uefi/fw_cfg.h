/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <microkit.h>

/*
 * Implements the QEMU Firmware Configuration (fw_cfg) Device
 * As described in https://github.com/qemu/qemu/blob/master/docs/specs/fw_cfg.rst
 */

/* Numerically defined keys.
 * From https://github.com/tianocore/edk2/blob/edk2-stable202605/OvmfPkg/Include/IndustryStandard/QemuFwCfg.h */
#define QEMU_FW_CFG_ITEM_SIGNATURE            0x0000
#define QEMU_FW_CFG_ITEM_INTERFACE_VERSION    0x0001
#define QEMU_FW_CFG_ITEM_SYSTEM_UUID          0x0002
#define QEMU_FW_CFG_ITEM_RAM_SIZE             0x0003
#define QEMU_FW_CFG_ITEM_GRAPHICS_ENABLED     0x0004
#define QEMU_FW_CFG_ITEM_SMP_CPU_COUNT        0x0005
#define QEMU_FW_CFG_ITEM_MACHINE_ID           0x0006
#define QEMU_FW_CFG_ITEM_KERNEL_ADDRESS       0x0007
#define QEMU_FW_CFG_ITEM_KERNEL_SIZE          0x0008
#define QEMU_FW_CFG_ITEM_KERNEL_COMMAND_LINE  0x0009
#define QEMU_FW_CFG_ITEM_INITRD_ADDRESS       0x000a
#define QEMU_FW_CFG_ITEM_INITRD_SIZE          0x000b
#define QEMU_FW_CFG_ITEM_BOOT_DEVICE          0x000c
#define QEMU_FW_CFG_ITEM_NUMA_DATA            0x000d
#define QEMU_FW_CFG_ITEM_BOOT_MENU            0x000e
#define QEMU_FW_CFG_ITEM_MAXIMUM_CPU_COUNT    0x000f
#define QEMU_FW_CFG_ITEM_KERNEL_ENTRY         0x0010
#define QEMU_FW_CFG_ITEM_KERNEL_DATA          0x0011
#define QEMU_FW_CFG_ITEM_INITRD_DATA          0x0012
#define QEMU_FW_CFG_ITEM_COMMAND_LINE_ADDRESS 0x0013
#define QEMU_FW_CFG_ITEM_COMMAND_LINE_SIZE    0x0014
#define QEMU_FW_CFG_ITEM_COMMAND_LINE_DATA    0x0015
#define QEMU_FW_CFG_ITEM_KERNEL_SETUP_ADDRESS 0x0016
#define QEMU_FW_CFG_ITEM_KERNEL_SETUP_SIZE    0x0017
#define QEMU_FW_CFG_ITEM_KERNEL_SETUP_DATA    0x0018
#define QEMU_FW_CFG_ITEM_FILE_DIR             0x0019 /* Also number of unamed selector keys */
#define QEMU_FW_CFG_FILE_FIRST                0x0020

#define QEMU_FW_CFG_FNAME_SIZE 56

/* Essential fw_cfg blobs for OVMF in QEMU_FW_CFG_ITEM_FILE_DIR */
/* https://github.com/tianocore/edk2/blob/b03a21a63e3bd001f52c527e5a57feddb53a690b/OvmfPkg/Library/PlatformInitLib/MemDetect.c#L401 */
#define E820_FWCFG_FILENAME "etc/e820"
/* https://github.com/tianocore/edk2/blob/f49f209c4f4c8b817d290f78e785099e8c51589f/OvmfPkg/Library/AcpiPlatformLib/QemuFwCfgAcpi.c#L1121 */
#define TABLE_LOADER_FWCFG_FILENAME "etc/table-loader"

/* Initialise the fw cfg interface and the architecture specific access handler. */
bool initialise_fw_cfg(void);

/* Add a named file to the fw cfg device, args are little endian. The function will
 * convert endianess internally according to the fw cfg spec.
 *
 * This function will copy `name` to an internal data structure, but it expect `data`
 * to be valid for the lifetime of the VMM. */
bool fw_cfg_add_named_file(char *name, size_t name_len, uint8_t *data, size_t data_size);

/* Add a legacy file to the fw cfg device, args are little endian. The function will
 * convert endianess internally according to the fw cfg spec.
 *
 * Legacy files have selector key between `QEMU_FW_CFG_ITEM_SYSTEM_UUID` and
 * `QEMU_FW_CFG_ITEM_KERNEL_SETUP_DATA`.
 *
 * This function expect `data` to be valid for the lifetime of the VMM. */
bool fw_cfg_add_file(uint16_t select_key, uint8_t *data, size_t data_size);
