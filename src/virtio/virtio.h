/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "virtio/mmio.h"

/*
 * All terminology used and functionality of the virtIO device implementation
 * adheres with the following specification:
 * Virtual I/O Device (VIRTIO) Version 1.2
 */

/* All the supported virtIO device types. */
enum virtio_device_type {
    CONSOLE,
    BLK,
};

bool virtio_mmio_device_init(virtio_device_t *dev,
                            enum virtio_device_type type,
                            uintptr_t region_base,
                            uintptr_t region_size,
                            size_t virq,
                            void **data_region_handles,
                            void **sddf_ring_handles,
                            size_t sddf_ch);
