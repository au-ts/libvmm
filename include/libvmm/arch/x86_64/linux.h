/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

uintptr_t linux_setup_images(uintptr_t ram_start,
                             size_t ram_size,
                             uintptr_t kernel,
                             size_t kernel_size,
                             uintptr_t initrd_src,
                             uintptr_t initrd_dest,
                             size_t initrd_size);
