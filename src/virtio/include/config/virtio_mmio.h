/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
// reference: https://docs.oasis-open.org/virtio/virtio/v1.1/csprd01/virtio-v1.1-csprd01.html

// table 4.1
#define VIRTIO_MMIO_DEV_MAGIC               0x74726976 // "virt"
#define VIRTIO_MMIO_DEV_VERSION		        0x2
#define VIRTIO_MMIO_DEV_VERSION_LEGACY		0x1
#define VIRTIO_MMIO_DEV_VERSION_LEGACY		0x1
#define VIRTIO_MMIO_DEV_VENDOR_ID           0x344c6573 // "seL4"

#define REG_VIRTIO_MMIO_MAGIC_VALUE		    0x000
#define REG_VIRTIO_MMIO_VERSION		        0x004
#define REG_VIRTIO_MMIO_DEVICE_ID		    0x008
#define REG_VIRTIO_MMIO_VENDOR_ID		    0x00c
#define REG_VIRTIO_MMIO_DEVICE_FEATURES	    0x010
#define REG_VIRTIO_MMIO_DEVICE_FEATURES_SEL 0x014
#define REG_VIRTIO_MMIO_DRIVER_FEATURES	    0x020
#define REG_VIRTIO_MMIO_DRIVER_FEATURES_SEL 0x024
#define REG_VIRTIO_MMIO_QUEUE_SEL		    0x030
#define REG_VIRTIO_MMIO_QUEUE_NUM_MAX	    0x034
#define REG_VIRTIO_MMIO_QUEUE_NUM		    0x038
#define REG_VIRTIO_MMIO_QUEUE_READY		    0x044
#define REG_VIRTIO_MMIO_QUEUE_NOTIFY	    0x050
#define REG_VIRTIO_MMIO_INTERRUPT_STATUS	0x060
#define REG_VIRTIO_MMIO_INTERRUPT_ACK	    0x064
#define REG_VIRTIO_MMIO_STATUS		        0x070
#define REG_VIRTIO_MMIO_QUEUE_DESC_LOW	    0x080
#define REG_VIRTIO_MMIO_QUEUE_DESC_HIGH	    0x084
#define REG_VIRTIO_MMIO_QUEUE_AVAIL_LOW	    0x090
#define REG_VIRTIO_MMIO_QUEUE_AVAIL_HIGH	0x094
#define REG_VIRTIO_MMIO_QUEUE_USED_LOW	    0x0a0
#define REG_VIRTIO_MMIO_QUEUE_USED_HIGH	    0x0a4
#define REG_VIRTIO_MMIO_CONFIG_GENERATION	0x0fc
#define REG_VIRTIO_MMIO_CONFIG		        0x100

// section 5
// The following device IDs are used to identify different types of virtio devices,
// only devices that sel4cp VMM currently supports are listed
#define DEVICE_ID_VIRTIO_NET          1
#define DEVICE_ID_VIRTIO_BLOCK        2
#define DEVICE_ID_VIRTIO_CONSOLE      3
#define DEVICE_ID_VIRTIO_GPU          16
#define DEVICE_ID_VIRTIO_VSOCK        19
