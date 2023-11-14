/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdint.h>
#include "util/util.h"
#include "virtio/virtq.h"
#include "serial/libserialsharedringbuffer/include/shared_ringbuffer.h"

// table 4.1
#define VIRTIO_MMIO_DEV_MAGIC               0x74726976 // "virt"
#define VIRTIO_MMIO_DEV_VERSION             0x2
#define VIRTIO_MMIO_DEV_VERSION_LEGACY      0x1
#define VIRTIO_MMIO_DEV_VERSION_LEGACY      0x1
#define VIRTIO_MMIO_DEV_VENDOR_ID           0x344c6573 // "seL4"

#define REG_VIRTIO_MMIO_MAGIC_VALUE         0x000
#define REG_VIRTIO_MMIO_VERSION             0x004
#define REG_VIRTIO_MMIO_DEVICE_ID           0x008
#define REG_VIRTIO_MMIO_VENDOR_ID           0x00c
#define REG_VIRTIO_MMIO_DEVICE_FEATURES     0x010
#define REG_VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014
#define REG_VIRTIO_MMIO_DRIVER_FEATURES     0x020
#define REG_VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024
#define REG_VIRTIO_MMIO_QUEUE_SEL           0x030
#define REG_VIRTIO_MMIO_QUEUE_NUM_MAX       0x034
#define REG_VIRTIO_MMIO_QUEUE_NUM           0x038
#define REG_VIRTIO_MMIO_QUEUE_READY         0x044
#define REG_VIRTIO_MMIO_QUEUE_NOTIFY        0x050
#define REG_VIRTIO_MMIO_INTERRUPT_STATUS    0x060
#define REG_VIRTIO_MMIO_INTERRUPT_ACK       0x064
#define REG_VIRTIO_MMIO_STATUS              0x070
#define REG_VIRTIO_MMIO_QUEUE_DESC_LOW      0x080
#define REG_VIRTIO_MMIO_QUEUE_DESC_HIGH     0x084
#define REG_VIRTIO_MMIO_QUEUE_AVAIL_LOW     0x090
#define REG_VIRTIO_MMIO_QUEUE_AVAIL_HIGH    0x094
#define REG_VIRTIO_MMIO_QUEUE_USED_LOW      0x0a0
#define REG_VIRTIO_MMIO_QUEUE_USED_HIGH     0x0a4
#define REG_VIRTIO_MMIO_CONFIG_GENERATION   0x0fc
#define REG_VIRTIO_MMIO_CONFIG              0x100

// section 5
// The following device IDs are used to identify different types of virtio devices,
// only devices that sel4cp VMM currently supports are listed
#define DEVICE_ID_VIRTIO_NET          1
#define DEVICE_ID_VIRTIO_BLOCK        2
#define DEVICE_ID_VIRTIO_CONSOLE      3
#define DEVICE_ID_VIRTIO_VSOCK        19

/* The maximum size (number of elements) of a virtqueue. It is set
 * to 128 because I copied it from the camkes virtio device. If you find
 * out that the virtqueue gets full easily, increase the number.
 */
#define QUEUE_SIZE 128

/* handler of a virtqueue */
// @ivanv: we can pack/bitfield this struct
typedef struct virtio_queue_handler {
    struct virtq virtq;
    /* is this virtq fully initialised? */
    bool ready;
    /* the last index that the virtIO device processed */
    uint16_t last_idx;
} virtio_queue_handler_t;

struct virtio_device;

// functions provided by the emul (device) layer for the emul (mmio) layer
typedef struct virtio_emul_funs {
    void (*device_reset)(struct virtio_device *dev);

    // REG_VIRTIO_MMIO_DEVICE_FEATURES related operations
    int (*get_device_features)(struct virtio_device *dev, uint32_t *features);
    int (*set_driver_features)(struct virtio_device *dev, uint32_t features);

    // REG_VIRTIO_MMIO_CONFIG related operations
    int (*get_device_config)(struct virtio_device *dev, uint32_t offset, uint32_t *ret_val);
    int (*set_device_config)(struct virtio_device *dev, uint32_t offset, uint32_t val);
    int (*queue_notify)(struct virtio_device *dev);
} virtio_device_funs_t;

// infomation you need for manipulating MMIO Device Register Layout
// @ivanv: I don't see why this extra struct is necessary, why not just make it part of the virtio device struct
// virtio_device?
typedef struct virtio_device_info {
    uint32_t DeviceID;
    uint32_t VendorID;

    uint32_t DeviceFeaturesSel;
    uint32_t DriverFeaturesSel;
    /* True if we are happy with what the driver requires */
    bool features_happy;

    uint32_t QueueSel;
    uint32_t QueueNotify;

    uint32_t InterruptStatus;

    uint32_t Status;

    // uint32_t ConfigGeneration;
} virtio_device_info_t;

/* Everything needed at runtime for a virtIO device to function. */
typedef struct virtio_device {
    virtio_device_info_t data;
    virtio_device_funs_t *funs;

    /* List of virt queues for the device */
    virtio_queue_handler_t *vqs;
    /* Length of the vqs list */
    size_t num_vqs;
    /* Virtual IRQ associated with this virtIO device */
    size_t virq;
    /* Handlers for sDDF ring buffers */
    void **sddf_rings;
    /* Microkit channel to the sDDF TX multiplexor */
    // @ivanv: this is microkit specific so maybe should be a callback instead or something.
    // @ivanv: my worry here is that the device struct is supposed to be for all devices, but
    // this is specific to device classes such as serial and networking
    // @ericc: on top of potentially changing to a callback, there can be multiple channels, one for each pair
    // of rings. For now im leaving it as one channel since that's the how much block, serial and ethernet use.
    size_t sddf_ch;
} virtio_device_t;

/**
 * Handles MMIO Device Register Layout I/O for VirtIO MMIO
 *
 * @param vcpu_id ID of the vcpu
 * @param fault_addr fault address
 * @param fsr fault status register
 * @param regs registers
 * @param data pointer to virtIO device registered with the fault handler
 */
bool virtio_mmio_fault_handle(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data);
