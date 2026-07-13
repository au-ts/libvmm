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
bool uefi_setup_images(uintptr_t firm_src, size_t firm_size, uint64_t flash_gpa, size_t flash_size);