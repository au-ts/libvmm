/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <libvmm/util/util.h>
#include <libvmm/virtio/virtq.h>

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

/* virtIO devices we have support for */
#define VIRTIO_DEVICE_ID_NET          1
#define VIRTIO_DEVICE_ID_BLOCK        2
#define VIRTIO_DEVICE_ID_CONSOLE      3
#define VIRTIO_DEVICE_ID_SOUND        25

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
    bool (*get_device_features)(struct virtio_device *dev, uint32_t *features);
    bool (*set_driver_features)(struct virtio_device *dev, uint32_t features);

    // REG_VIRTIO_MMIO_CONFIG related operations
    bool (*get_device_config)(struct virtio_device *dev, uint32_t offset, uint32_t *ret_val);
    bool (*set_device_config)(struct virtio_device *dev, uint32_t offset, uint32_t val);
    bool (*queue_notify)(struct virtio_device *dev);
} virtio_device_funs_t;

/* Emulated MMIO registers for the virtIO device */
typedef struct virtio_device_regs {
    uint32_t DeviceID;
    uint32_t VendorID;

    uint32_t DeviceFeaturesSel;
    uint32_t DriverFeaturesSel;

    uint32_t QueueSel;
    uint32_t QueueNotify;

    uint32_t InterruptStatus;

    uint32_t Status;

    uint32_t ConfigGeneration;
} virtio_device_regs_t;

/* Everything needed at runtime for a virtIO device to function. */
typedef struct virtio_device {
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
