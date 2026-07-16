/*
 * Copyright 2026, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <microkit.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/* Loads the given UEFI firmware into the guest's "flash" region. */
bool uefi_setup_images(uintptr_t firm_src, size_t firm_size, uintptr_t dsdt_src, size_t dsdt_size, uint64_t flash_gpa,
                       size_t flash_size);

/* Add a Linux kernel and initrd as a boot option for the UEFI firmware. Expects that the lifetime of the
 * kernel and initrd buffers to be static. */
bool uefi_add_linux_boot(uintptr_t kernel_src, size_t kernel_size, uintptr_t initrd_src, size_t initrd_size,
                         char *cmdline);