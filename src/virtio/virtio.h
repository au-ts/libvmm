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
};

bool virtio_mmio_device_init(virtio_device_t *dev,
                            enum virtio_device_type type,
                            uintptr_t region_base,
                            uintptr_t region_size,
                            size_t virq,
                            serial_queue_handle_t *sddf_rx_queue,
                            serial_queue_handle_t *sddf_tx_queue,
                            size_t sddf_mux_tx_ch);
