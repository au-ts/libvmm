
/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/util/util.h>

bool uefi_setup_images(uintptr_t ram_start_vmm, uint64_t ram_start_gpa, size_t ram_size, uintptr_t high_ram_start_vmm,
                       uint64_t high_ram_start_gpa, size_t high_ram_size, uintptr_t flash_start_vmm,
                       uintptr_t flash_start_gpa, size_t flash_size, char *firm, size_t firm_size, void *dsdt_blob,
                       uint64_t dsdt_blob_size);