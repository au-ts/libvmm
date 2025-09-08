/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Check that the given Linux images that are required on ARM/RISC-V
 * live within the guest RAM provided and do not overlap each other.
 * Useful for sanity checking before debugging strange errors.
 */
bool linux_validate_image_locations(uintptr_t ram_start,
                                    size_t ram_size,
                                    uintptr_t kernel,
                                    size_t kernel_size,
                                    uintptr_t dtb_dest,
                                    size_t dtb_size,
                                    uintptr_t initrd_dest,
                                    size_t initrd_size);
