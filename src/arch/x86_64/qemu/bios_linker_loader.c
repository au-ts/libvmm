/* Dynamic linker/loader of ACPI tables
 *
 * Copyright (C) 2013 Red Hat Inc
 *
 * Author: Michael S. Tsirkin <mst@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <stdbool.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/qemu/bios_linker_loader.h>
#include <sddf/util/custom_libc/string.h>

// These creating functions have been modified to fit with our logic better.

void bios_linker_loader_alloc(const char *file_name, uint32_t alloc_align, bool alloc_fseg,
                              struct BiosLinkerLoaderEntry *ret)
{
    assert(ret);
    assert(file_name);
    assert(!(alloc_align & (alloc_align - 1)));

    memset(ret, 0, sizeof(struct BiosLinkerLoaderEntry));
    strcpy(ret->alloc.file, file_name);
    ret->command = BIOS_LINKER_LOADER_COMMAND_ALLOCATE;
    ret->alloc.align = alloc_align;
    ret->alloc.zone = alloc_fseg ? BIOS_LINKER_LOADER_ALLOC_ZONE_FSEG : BIOS_LINKER_LOADER_ALLOC_ZONE_HIGH;
}

void bios_linker_loader_add_pointer(const char *dest_file_name, char *dest_file_blob, uint32_t dest_file_blob_size,
                                    uint32_t dst_patched_offset, uint8_t dst_patched_size, const char *src_file_name,
                                    uint32_t src_offset, struct BiosLinkerLoaderEntry *ret)
{
    assert(dest_file_name);
    assert(dest_file_blob);
    assert(src_file_name);
    assert(dst_patched_offset < dest_file_blob_size);
    assert(dst_patched_offset + dst_patched_size <= dest_file_blob_size);

    memset(ret, 0, sizeof(struct BiosLinkerLoaderEntry));
    strcpy(ret->pointer.dest_file, dest_file_name);
    strcpy(ret->pointer.src_file, src_file_name);
    ret->command = BIOS_LINKER_LOADER_COMMAND_ADD_POINTER;
    ret->pointer.offset = dst_patched_offset;
    ret->pointer.size = dst_patched_size;
    assert(dst_patched_size == 1 || dst_patched_size == 2 || dst_patched_size == 4 || dst_patched_size == 8);

    memcpy(dest_file_blob + dst_patched_offset, &src_offset, dst_patched_size);
}

void bios_linker_loader_add_checksum(const char *file_name, char *file_blob, uint32_t file_blob_size,
                                     uint32_t start_offset, uint32_t size, uint32_t checksum_offset,
                                     struct BiosLinkerLoaderEntry *ret)
{
    assert(file_name);
    assert(start_offset < file_blob_size);
    assert(start_offset + size <= file_blob_size);
    assert(checksum_offset >= start_offset);
    assert(checksum_offset + 1 <= start_offset + size);

    *(file_blob + checksum_offset) = 0;
    memset(ret, 0, sizeof(struct BiosLinkerLoaderEntry));
    strcpy(ret->cksum.file, file_name);
    ret->command = BIOS_LINKER_LOADER_COMMAND_ADD_CHECKSUM;
    ret->cksum.offset = checksum_offset;
    ret->cksum.start = start_offset;
    ret->cksum.length = size;
}
