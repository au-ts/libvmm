/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdint.h>
#include "../util/util.h"
#include "include/config/virtio_mmio.h"
#include "include/vring/virtio_ring.h"

/* Random numbers that I picked, if it overlaps with other memory regions,
 * change this to use another slot. You might also need to change:
 * 1. VIRTIO_<device name>_ADDRESS_START
 * 2. VIRTIO_<device name>_ADDRESS_END
 * 3. The virtio device entry in your DTS file
 */
#define VIRTIO_ADDRESS_START    0x130000
#define VIRTIO_ADDRESS_END      0x230000

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

struct virtio_emul_handler;

// functions provided by the emul (device) layer for the emul (mmio) layer
typedef struct virtio_emul_funs {
    void (*device_reset)(struct virtio_emul_handler *self);

    // REG_VIRTIO_MMIO_DEVICE_FEATURES related operations
    int (*get_device_features)(struct virtio_emul_handler *self, uint32_t *features);
    int (*set_driver_features)(struct virtio_emul_handler *self, uint32_t features);

    // REG_VIRTIO_MMIO_CONFIG related operations
    int (*get_device_config)(struct virtio_emul_handler *self, uint32_t offset, uint32_t *ret_val);
    int (*set_device_config)(struct virtio_emul_handler *self, uint32_t offset, uint32_t val);
    int (*queue_notify)(struct virtio_emul_handler *self);
} virtio_emul_funs_t;

// infomation you need for manipulating MMIO Device Register Layout
typedef struct virtio_emul_info {
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
} virtio_emul_info_t;

// should only contain guest-emul information
typedef struct virtio_emul_handler {
    virtio_emul_info_t data;
    virtio_emul_funs_t *funs;

    // pointer to a list of virtqueues, the length of the list is depended on the type of the virtio device
    virtqueue_t *vqs;
} virtio_emul_handler_t;

/**
 * Handles MMIO Device Register Layout I/O for VirtIO MMIO
 *
 * @param vcpu_id ID of the vcpu
 * @param fault_addr fault address
 * @param fsr fault status register
 * @param regs registers
 */
bool handle_virtio_mmio_fault(uint64_t vcpu_id, uint64_t fault_addr, uint64_t fsr, seL4_UserContext *regs);
