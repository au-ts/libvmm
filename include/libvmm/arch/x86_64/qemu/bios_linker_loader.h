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

#pragma once

#include <stdint.h>

#define FW_CFG_MAX_FILE_PATH 56
#define BIOS_LINKER_LOADER_FILESZ FW_CFG_MAX_FILE_PATH

struct BiosLinkerLoaderEntry {
    uint32_t command;
    union {
        /*
         * COMMAND_ALLOCATE - allocate a table from @alloc.file
         * subject to @alloc.align alignment (must be power of 2)
         * and @alloc.zone (can be HIGH or FSEG) requirements.
         *
         * Must appear exactly once for each file, and before
         * this file is referenced by any other command.
         */
        struct {
            char file[BIOS_LINKER_LOADER_FILESZ];
            uint32_t align;
            uint8_t zone;
        } alloc;

        /*
         * COMMAND_ADD_POINTER - patch the table (originating from
         * @dest_file) at @pointer.offset, by adding a pointer to the table
         * originating from @src_file. 1,2,4 or 8 byte unsigned
         * addition is used depending on @pointer.size.
         */
        struct {
            char dest_file[BIOS_LINKER_LOADER_FILESZ];
            char src_file[BIOS_LINKER_LOADER_FILESZ];
            uint32_t offset;
            uint8_t size;
        } pointer;

        /*
         * COMMAND_ADD_CHECKSUM - calculate checksum of the range specified by
         * @cksum_start and @cksum_length fields,
         * and then add the value at @cksum.offset.
         * Checksum simply sums -X for each byte X in the range
         * using 8-bit math.
         */
        struct {
            char file[BIOS_LINKER_LOADER_FILESZ];
            uint32_t offset;
            uint32_t start;
            uint32_t length;
        } cksum;

        /*
         * COMMAND_WRITE_POINTER - write the fw_cfg file (originating from
         * @dest_file) at @wr_pointer.offset, by adding a pointer to
         * @src_offset within the table originating from @src_file.
         * 1,2,4 or 8 byte unsigned addition is used depending on
         * @wr_pointer.size.
         */
        struct {
            char dest_file[BIOS_LINKER_LOADER_FILESZ];
            char src_file[BIOS_LINKER_LOADER_FILESZ];
            uint32_t dst_offset;
            uint32_t src_offset;
            uint8_t size;
        } wr_pointer;

        /* padding */
        char pad[124];
    };
} __attribute__((packed));
typedef struct BiosLinkerLoaderEntry BiosLinkerLoaderEntry;

enum {
    BIOS_LINKER_LOADER_COMMAND_ALLOCATE = 0x1,
    BIOS_LINKER_LOADER_COMMAND_ADD_POINTER = 0x2,
    BIOS_LINKER_LOADER_COMMAND_ADD_CHECKSUM = 0x3,
    BIOS_LINKER_LOADER_COMMAND_WRITE_POINTER = 0x4,
};

enum {
    BIOS_LINKER_LOADER_ALLOC_ZONE_HIGH = 0x1,
    BIOS_LINKER_LOADER_ALLOC_ZONE_FSEG = 0x2,
};

/*
 * Linker/loader is a paravirtualized interface that passes commands to guest.
 * The commands can be used to request guest to
 * - allocate memory chunks and initialize them from QEMU FW CFG files
 * - link allocated chunks by storing pointer to one chunk into another
 * - calculate ACPI checksum of part of the chunk and store into same chunk
 */

/*
 * bios_linker_loader_alloc: ask guest to load file into guest memory.
 *
 * @file_name: name of the file blob to be loaded
 * @alloc_align: required minimal alignment in bytes. Must be a power of 2.
 * @alloc_fseg: request allocation in FSEG zone (useful for the RSDP ACPI table)
 * @ret: returns this command's entry
 *
 * Note: this command must precede any other linker command using this file.
 */
void bios_linker_loader_alloc(const char *file_name, uint32_t alloc_align, bool alloc_fseg,
                              struct BiosLinkerLoaderEntry *ret);

/*
 * bios_linker_loader_add_pointer: ask guest to patch address in
 * destination file with a pointer to source file
 * @dest_file_name: destination file name that must be changed
 * @dest_file_blob: destination file data that must be changed
 * @dest_file_blob_size: destination file data whole size
 * @dst_patched_offset: location within destination file blob to be patched
 *                      with the pointer to @src_file+@src_offset (i.e. source
 *                      blob allocated in guest memory + @src_offset), in bytes
 * @dst_patched_offset_size: size of the pointer to be patched
 *                      at @dst_patched_offset in @dest_file blob, in bytes
 * @src_file_name: source file who's address must be taken
 * @src_offset: location within source file blob to which
 *              @dest_file+@dst_patched_offset will point to after
 *              firmware's executed ADD_POINTER command
 * @ret: returns this command's entry
 */
void bios_linker_loader_add_pointer(const char *dest_file_name, char *dest_file_blob, uint32_t dest_file_blob_size,
                                    uint32_t dst_patched_offset, uint8_t dst_patched_size, const char *src_file_name,
                                    uint32_t src_offset, struct BiosLinkerLoaderEntry *ret);

/*
 * bios_linker_loader_add_checksum: ask guest to add checksum of ACPI
 * table in the specified file at the specified offset.
 *
 * Checksum calculation simply sums -X for each byte X in the range
 * using 8-bit math (i.e. ACPI checksum).
 *
 * @file_name: file that includes the checksum to be calculated
 *             and the data to be checksummed
 * @file_blob: data of said file
 * @file_blob_size: size of said file
 * @start_offset, @size: range of data in the file to checksum,
 *                       relative to the start of file blob
 * @checksum_offset: location of the checksum to be patched within file blob,
 *                   relative to the start of file blob
 * @ret: returns this command's entry
 */
void bios_linker_loader_add_checksum(const char *file_name, char *file_blob, uint32_t file_blob_size,
                                     uint32_t start_offset, uint32_t size, uint32_t checksum_offset,
                                     struct BiosLinkerLoaderEntry *ret);