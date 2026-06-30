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

#define VIRTIO_DEV_VENDOR_ID                0x344c6573 // "seL4"

// table 4.1
#define VIRTIO_MMIO_DEV_MAGIC               0x74726976 // "virt"
#define VIRTIO_MMIO_DEV_VERSION             0x2
#define VIRTIO_MMIO_DEV_VERSION_LEGACY      0x1

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

typedef struct virtio_mmio_data {
    uint32_t revision;
    uint32_t vendor_id;
} virtio_mmio_data_t;

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
