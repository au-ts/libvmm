
/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/util/util.h>

bool uefi_setup_images(uintptr_t ram_start, size_t ram_size, uintptr_t flash_start, size_t flash_size, char *firm,
                       size_t firm_size, void *dsdt_blob, uint64_t dsdt_blob_size);