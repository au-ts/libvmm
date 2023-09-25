/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * Generic VirtIO MMIO handlers for virtio devices that use mmio transport mode
 */

#pragma once

#include <stdint.h>
#include "../util/util.h"
#include "include/config/virtio_mmio.h"
#include "include/vring/virtio_ring.h"

/* The maximum size (number of elements) of a virtqueue. It is set
 * to 128 because I copied it from the camkes virtio backend. If you find
 * out that the virtqueue gets full easily, increase the number.
 */
#define QUEUE_SIZE 128

/* handler of a virtqueue */
typedef struct v_queue {
    struct vring vring;
    uint16_t last_idx;  /* the last index that the backend processed */
    bool ready;         /* is this vring fully initialised? */
} virtqueue_t;

struct virtio_mmio_handler;

// functions provided by the generic mmio layer to a device specific layer
typedef struct virtio_mmio_funs {
    void (*device_reset)(void);

    // REG_VIRTIO_MMIO_DEVICE_FEATURES related operations
    int (*get_device_features)(uint32_t *features);
    int (*set_driver_features)(uint32_t features);

    // REG_VIRTIO_MMIO_CONFIG related operations
    int (*get_device_config)(uint32_t offset, uint32_t *ret_val);
    int (*set_device_config)(uint32_t offset, uint32_t val);
    int (*queue_notify)(void);
} virtio_mmio_funs_t;

// infomation you need for manipulating MMIO Device Register Layout
typedef struct virtio_mmio_info {
    uint32_t DeviceID;
    uint32_t VendorID;

    uint32_t DeviceFeaturesSel;
    uint32_t DriverFeaturesSel;
    bool features_happy;    /* True if we are happy with what the driver requires */

    uint32_t QueueSel;
    uint32_t QueueNotify;

    uint32_t InterruptStatus;

    uint32_t Status;

    // uint32_t ConfigGeneration;
} virtio_mmio_info_t;

// should only contain guest information
typedef struct virtio_mmio_handler {
    virtio_mmio_info_t data;
    virtio_mmio_funs_t *funs;

    // pointer to a list of virtqueues, the length of the list is depended on the type of the virtio device
    virtqueue_t *vqs;
} virtio_mmio_handler_t;

/**
 * Handles MMIO Device Register Layout I/O for VirtIO MMIO
 *
 * @param vcpu_id ID of the vcpu
 * @param fault_addr fault address
 * @param fsr fault status register
 * @param regs registers
 */
bool virtio_mmio_handle_fault(size_t vcpu_id, uint64_t fault_addr, uint64_t fsr, seL4_UserContext *regs);