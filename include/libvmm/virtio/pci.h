/*
 * Copyright 2025, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <libvmm/pci.h>
#include <libvmm/util/util.h>

/* PCI REVISION */
#define VIRTIO_PCI_REVISION 0

/* Common configuration */
#define VIRTIO_PCI_CAP_COMMON_CFG 1
/* Notifications */
#define VIRTIO_PCI_CAP_NOTIFY_CFG 2
/* ISR Status */
#define VIRTIO_PCI_CAP_ISR_CFG 3
/* Device specific configuration */
#define VIRTIO_PCI_CAP_DEVICE_CFG 4
/* PCI configuration access */
#define VIRTIO_PCI_CAP_PCI_CFG 5
/* Shared memory region */
#define VIRTIO_PCI_CAP_SHARED_MEMORY_CFG 8
/* Vendor-specific data */
#define VIRTIO_PCI_CAP_VENDOR_CFG 9

// PCI Common configuration
#define VIRTIO_PCI_COMMON_DEV_FEATURE_SEL       0x0
#define VIRTIO_PCI_COMMON_DEV_FEATURE           0x4
#define VIRTIO_PCI_COMMON_DRI_FEATURE_SEL       0x8
#define VIRTIO_PCI_COMMON_DRI_FEATURE           0xc
#define VIRTIO_PCI_COMMON_MSIX                  0x10
#define VIRTIO_PCI_COMMON_NUM_QUEUES            0x12
#define VIRTIO_PCI_COMMON_DEV_STATUS            0x14
#define VIRTIO_PCI_COMMON_CFG_GENERATION        0x15
#define VIRTIO_PCI_COMMON_Q_SELECT              0x16
#define VIRTIO_PCI_COMMON_Q_SIZE                0x18
#define VIRTIO_PCI_COMMON_Q_MSIX                0x1a
#define VIRTIO_PCI_COMMON_Q_ENABLE              0x1c
#define VIRTIO_PCI_COMMON_Q_NOTIF_OFF           0x1e
#define VIRTIO_PCI_COMMON_Q_DESC_LO             0x20
#define VIRTIO_PCI_COMMON_Q_DESC_HI             0x24
#define VIRTIO_PCI_COMMON_Q_AVAIL_LO            0x28
#define VIRTIO_PCI_COMMON_Q_AVAIL_HI            0x2c
#define VIRTIO_PCI_COMMON_Q_USED_LO             0x30
#define VIRTIO_PCI_COMMON_Q_USED_HI             0x34
#define VIRTIO_PCI_COMMON_Q_NOTIF_DATA          0x38
#define VIRTIO_PCI_COMMON_Q_RESET               0x3a
#define VIRTIO_PCI_COMMON_ADM_Q_IDX             0x3c
#define VIRTIO_PCI_COMMON_ADM_Q_NUM             0x3e
#define VIRTIO_PCI_COMMON_END                   0x40

#define VIRTIO_PCI_VENDOR_ID            0x1AF4
#define VIRTIO_PCI_NET_DEV_ID           0x1000
#define VIRTIO_PCI_BLK_DEV_ID           0x1001
#define VIRTIO_PCI_CONSOLE_DEV_ID       0x1003
#define VIRTIO_PCI_NOTIF_OFF_MULTIPLIER 0x2
#define VIRTIO_PCI_QUEUE_NUM_MAX        0x2

typedef struct virtio_pci_data {
    uint32_t device_id;
    uint32_t vendor_id;
    uint32_t device_class;
    pci_dev_handle_t pci_handle;
} virtio_pci_data_t;

// Vendor-specific Capability (ID 0x09)
struct virtio_pci_cap {
    uint8_t cap_len;              /* capability length. */
    uint8_t cfg_type;             /* Identifies the structure. */
    uint8_t bar;                  /* Where to find it. */
    uint8_t id;                   /* Multiple capabilities of the same type */
    uint8_t padding[2];           /* Pad to full dword. */
    uint32_t offset;              /* Offset within bar. */
    uint32_t length;              /* Length of the structure, in bytes. */
} __attribute__((packed));

/* PCI Notify capability */
struct virtio_pci_notify_cap {
    struct virtio_pci_cap cap;
    uint32_t notify_off_multiplier; /* Multiplier for queue_notify_off. */
} __attribute__((packed));

/* PCI CFG capability: provides indirect access */
struct virtio_pci_cfg_cap {
    struct virtio_pci_cap cap;
    uint32_t pci_cfg_data;
} __attribute__((packed));

struct virtio_pci_common_cfg {
    /* About the whole device. */
    uint32_t device_feature_select;     /* read-write */
    uint32_t device_feature;            /* read-only for driver */
    uint32_t driver_feature_select;     /* read-write */
    uint32_t driver_feature;            /* read-write */
    uint16_t config_msix_vector;        /* read-write */
    uint16_t num_queues;                /* read-only for driver */
    uint8_t device_status;              /* read-write */
    uint8_t config_generation;          /* read-only for driver */

    /* About a specific virtqueue. */
    uint16_t queue_select;              /* read-write */
    uint16_t queue_size;                /* read-write */
    uint16_t queue_msix_vector;         /* read-write */
    uint16_t queue_enable;              /* read-write */
    uint16_t queue_notify_off;          /* read-only for driver */
    uint64_t queue_desc;                /* read-write */
    uint64_t queue_driver;              /* read-write */
    uint64_t queue_device;              /* read-write */
    uint16_t queue_notif_config_data;   /* read-only for driver */
    uint16_t queue_reset;               /* read-write */

    /* About the administration virtqueue. */
    uint16_t admin_queue_index;         /* read-only for driver */
    uint16_t admin_queue_num;           /* read-only for driver */
};

typedef struct virtio_device virtio_device_t;

/*
 * Registers a new virtIO device at a given guest-physical region.
 *
 * Assumes the virtio_device_t *dev struct passed has been populated
 * and virtual IRQ associated with the device has been registered.
 */
bool virtio_pci_register_device(virtio_device_t *dev, uint16_t pci_bus, uint16_t pci_dev, int virq);
