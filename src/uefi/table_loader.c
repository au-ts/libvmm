/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 */

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <microkit.h>
#include <libvmm/libvmm.h>
#include <libvmm/uefi/table_loader.h>

static bool is_pow2_alignment(uint32_t alignment)
{
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        LOG_VMM_ERR("alignment %u is not a power of 2!\n", alignment);
        return false;
    }
    return true;
}

bool table_loader_allocate(qemu_loader_entry_t *entry, char *file_name, uint32_t alignment, bool use_fseg)
{
    if (!entry) {
        LOG_VMM_ERR("entry is NULL!\n");
        return false;
    }

    if (!file_name) {
        LOG_VMM_ERR("file_name is NULL!\n");
        return false;
    }

    if (!is_pow2_alignment(alignment)) {
        return false;
    }

    memset(entry, 0, sizeof(qemu_loader_entry_t));

    entry->type = qemu_loader_cmd_allocate;
    entry->command.allocate.alignment = alignment;
    entry->command.allocate.zone = use_fseg ? qemu_loader_alloc_fseg : qemu_loader_alloc_high;

    strcpy((char *)&entry->command.allocate.file[0], file_name);

    return true;
}

bool table_loader_add_pointer(qemu_loader_entry_t *entry, char *dest_file, char *src_file, void *dest_blob,
                              uint32_t patch_offset, uint8_t patch_size, uint32_t src_offset)
{
    if (!entry) {
        LOG_VMM_ERR("entry is NULL!\n");
        return false;
    }

    if (!dest_file) {
        LOG_VMM_ERR("dest_file is NULL!\n");
        return false;
    }

    if (!src_file) {
        LOG_VMM_ERR("src_file is NULL!\n");
        return false;
    }

    if (!dest_blob) {
        LOG_VMM_ERR("dest_blob is NULL!\n");
        return false;
    }

    if (patch_size == 1 || patch_size == 2 || patch_size == 4 || patch_size == 8) {
        memset(entry, 0, sizeof(qemu_loader_entry_t));

        entry->type = qemu_loader_cmd_add_pointer;
        entry->command.add_pointer.pointer_offset = patch_offset;
        entry->command.add_pointer.pointer_size = patch_size;

        strcpy((char *)&entry->command.add_pointer.pointer_file[0], dest_file);
        strcpy((char *)&entry->command.add_pointer.pointee_file[0], src_file);

        /* Write the source offset directly into the destination blob */
        memcpy((uint8_t *)dest_blob + patch_offset, &src_offset, patch_size);

        return true;
    } else {
        LOG_VMM_ERR("patch_size %u is not 1, 2, 4 or 8!\n", patch_size);
        return false;
    }
}

bool table_loader_add_checksum(qemu_loader_entry_t *entry, char *file_name, void *blob, uint32_t start_offset,
                               uint32_t length, uint32_t checksum_offset)
{
    if (!entry) {
        LOG_VMM_ERR("entry is NULL!\n");
        return false;
    }

    if (!file_name) {
        LOG_VMM_ERR("file_name is NULL!\n");
        return false;
    }

    if (!blob) {
        LOG_VMM_ERR("blob is NULL!\n");
        return false;
    }

    if (!length) {
        LOG_VMM_ERR("length is 0!\n");
        return false;
    }

    // @billn check file name length, need to update sddf to include strnlen or something like taht

    memset(entry, 0, sizeof(qemu_loader_entry_t));

    entry->type = qemu_loader_cmd_add_checksum;
    entry->command.add_checksum.start = start_offset;
    entry->command.add_checksum.length = length;
    entry->command.add_checksum.result_offset = checksum_offset;

    strcpy((char *)&entry->command.add_checksum.file[0], file_name);

    ((uint8_t *)blob)[checksum_offset] = 0;

    return true;
}