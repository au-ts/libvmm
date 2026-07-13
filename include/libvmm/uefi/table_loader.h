/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * The struct definitions in this file is a reimplementation from an equivalent file in EDK II:
 * https://github.com/tianocore/edk2/blob/edk2-stable202605/OvmfPkg/Include/IndustryStandard/QemuLoader.h
 *
 * Command structures for the QEMU FwCfg table loader interface.
 * Copyright (C) 2014, Red Hat, Inc.
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <microkit.h>
#include <libvmm/uefi/fw_cfg.h>

/* This inferface is used to pass commands to the UEFI firmware in the guest for:
 * - allocating buffers and filling them from QEMU Fw Cfg files,
 * - link the buffers by writing the pointer of one to another, and
 * - calculate ACPI checksums of part of the buffer. */

/* The types and the documentation reflects the SeaBIOS interface. */
#define QEMU_LOADER_FNAME_SIZE QEMU_FW_CFG_FNAME_SIZE

typedef enum {
    qemu_loader_cmd_allocate = 1,
    qemu_loader_cmd_add_pointer,
    qemu_loader_cmd_add_checksum,
    qemu_loader_cmd_write_pointer,
} qemu_loader_cmd_type_t;

typedef enum { qemu_loader_alloc_high = 1, qemu_loader_alloc_fseg } qemu_loader_alloc_zone_t;

#pragma pack(1)
/* qemu_loader_cmd_allocate: download the fw_cfg file named `file`, to a buffer
 * allocated in the zone specified by `zone`, aligned at a multiple of `alignment`.
 */
typedef struct {
    uint8_t file[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated
    uint32_t alignment;                   // power of two
    uint8_t zone;                         // qemu_loader_alloc_zone_t values
} qemu_loader_allocate_t;

/* qemu_loader_cmd_add_pointer: the bytes at
 * [pointer_dffset..pointer_dffset+pointer_size) in the file pointer_file contain a
 * relative pointer (an offset) into pointee_file. Increment the relative
 * pointer's value by the base address of where pointer_file's contents have
 * been placed (when qemu_loader_cmd_allocate has been executed for pointee_file).
 */
typedef struct {
    uint8_t pointer_file[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated
    uint8_t pointee_file[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated
    uint32_t pointer_offset;
    uint8_t pointer_size;                         // one of 1, 2, 4, 8
} qemu_loader_add_pointer_t;

/* qemu_loader_cmd_add_checksum: calculate the uint8_t checksum (as per
 * CalculateChecksum8()) of the range [`start`..`start`+`length`) in `file`. Store the
 * uint8_t result at result_offset in the same File. */
typedef struct {
    uint8_t file[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated
    uint32_t result_offset;
    uint32_t start;
    uint32_t length;
} qemu_loader_add_checksum_t;

/* qemu_loader_cmd_write_pointer: the bytes at
 * [pointer_offset..pointer_offset+pointer_size) in the writeable fw_cfg file
 * pointer_file are to receive the absolute address of pointee_file, as allocated
 * and downloaded by the firmware, incremented by the value of pointee_offset.
 * Store the sum of (a) the base address of where pointee_file's contents have
 * been placed (when qemu_loader_cmd_allocate has been executed for pointee_file)
 * and (b) pointee_offset, to this portion of pointer_file.
 *
 * This command is similar to qemu_loader_cmd_add_pointer; the difference is that
 * the "pointer to patch" does not exist in guest-physical address space, only
 * in "fw_cfg file space". In addition, the "pointer to patch" is not
 * initialized by QEMU in-place with a possibly nonzero offset value: the
 * relative offset into pointee_file comes from the explicit pointee_offset
 * field.
 */
typedef struct {
    uint8_t pointer_file[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated, "destination"
    uint8_t pointee_file[QEMU_LOADER_FNAME_SIZE]; // NUL-terminated, "source"
    uint32_t pointer_offset;
    uint32_t pointee_offset;
    uint8_t pointer_size;                      // one of 1, 2, 4, 8
} qemu_loader_write_pointer_t;

typedef struct {
    uint32_t type;                          // qemu_loader_cmd_type_t values
    union {
        qemu_loader_allocate_t allocate;
        qemu_loader_add_pointer_t add_pointer;
        qemu_loader_add_checksum_t add_checksum;
        qemu_loader_write_pointer_t write_pointer;
        uint8_t padding[124];
    } command;
} qemu_loader_entry_t;

#pragma pack()

/* Make a qemu_loader_cmd_allocate command
 *
 * Note: this command must precede any other linker command using this file.
 */
bool table_loader_allocate(qemu_loader_entry_t *entry, char *file_name, uint32_t alignment, bool use_fseg);

/*
 * Make a qemu_loader_cmd_add_pointer command
 */
bool table_loader_add_pointer(qemu_loader_entry_t *entry, char *dest_file, char *src_file, void *dest_blob,
                              uint32_t patch_offset, uint8_t patch_size, uint32_t src_offset);

/*
 * Make a qemu_loader_cmd_add_checksum command
 */
bool table_loader_add_checksum(qemu_loader_entry_t *entry, char *file_name, void *blob, uint32_t start_offset,
                               uint32_t length, uint32_t checksum_offset);