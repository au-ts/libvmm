/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <libvmm/virtio/mmio.h>
#include <libvmm/virtio/pci.h>

/*
 * All terminology used and functionality of the virtIO device implementation
 * adheres with the following specification:
 * Virtual I/O Device (VIRTIO) Version 1.2
 */

typedef enum virtio_transport_type {
    VIRTIO_TRANSPORT_PCI,
    VIRTIO_TRANSPORT_MMIO,
    VIRTIO_TRANSPORT_CCW,    // For future expansion
} virtio_transport_type_t;

typedef union virtio_transport_data {
    virtio_pci_data_t pci;
    virtio_mmio_data_t mmio;
} virtio_transport_data_t;

/* Everything needed at runtime for a virtIO device to function. */
typedef struct virtio_device {
    virtio_transport_type_t transport_type;
    virtio_transport_data_t transport;
    virtio_device_regs_t regs;
    virtio_device_funs_t *funs;
    /* List of virt queues for the device */
    virtio_queue_handler_t *vqs;
    /* Length of the vqs list */
    size_t num_vqs;
    /* Virtual IRQ associated with this virtIO device */
    size_t virq;
    /* Device specific data such as sDDF queues */
    void *device_data;
    /* True if we are happy with what the driver requires */
    bool features_happy;
} virtio_device_t;

/*
 * Registers a new virtIO device at a given guest-physical region.
 *
 * Assumes the virtio_device_t *dev struct passed has been populated
 * and virtual IRQ associated with the device has been registered.
 */
bool virtio_mmio_register_device(virtio_device_t *dev,
                                 uintptr_t region_base,
                                 uintptr_t region_size,
                                 size_t virq);
