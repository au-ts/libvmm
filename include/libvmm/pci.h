/*
 * Copyright 2026, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <libvmm/virq.h>
#include <stdint.h>

/* Implements a guest-side virtual PCI bus and an interface that virtual PCI devices
 * must implements to expose itself to the guest. */

// PCI Command Register
#define PCI_COMMAND_MEMORY             0x2    /* Enable response in Memory space */
#define PCI_COMMAND_MASTER             0x4    /* Enable bus mastering */

// PCI Status Register
#define PCI_STATUS_CAP_LIST             0x10    /* Support Capability List */

// PCI Class
#define PCI_CLASS_STORAGE_SCSI           0x0100
#define PCI_CLASS_NETWORK_ETHERNET       0x0200
#define PCI_CLASS_COMMUNICATION_OTHER    0x0780

#define PCI_CLASS_CODE(x) ((x >> 8) & 0xFF)
#define PCI_SUB_CLASS(x) (x & 0xFF)

// PCI Capability IDs
#define PCI_CAP_ID_VNDR     0x09    // Vendor Specific

/* Initialise the guest visible virtual PCI bus. The MMIO aperature must be the same as what you told the guest
 * via the DTS on ARM or DSDT on x86. */
bool pci_bus_init(uint64_t ecam_gpa, uint32_t ecam_size, uint64_t mmio_aperature_gpa, uint64_t mmio_aperature_size);

/* Information about the device during the initial registration process. */
typedef struct pci_device_register_data {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t subclass;
    uint8_t class_code;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_device_id;

    // @billn todo, add callbacks for reading status registers etc?
} pci_device_register_data_t;

#define INVALID_PCI_DEVICE_HANDLE -1
typedef int pci_dev_handle_t;

/* Create a virtual PCI device on the virtual PCI bus. */
pci_dev_handle_t pci_register_device(uint8_t bus, uint8_t dev, uint8_t func, pci_device_register_data_t *device_data);

/* Register the virtual PCI device IRQ with the virtual IRQ controller. */
bool pci_register_device_irq(pci_dev_handle_t pci_dev_handle, int virq, virq_ack_fn_t ack_fn, void *ack_data);

/* Inject an IRQ into the guest from the given virtual PCI device. The IRQ must be registered first. */
bool pci_inject_device_irq(pci_dev_handle_t pci_dev_handle);

/* Add a capability to a virtual PCI device capability linked list. A PCI device capability
 * looks like this and is tightly packed:
struct pci_capability {
    struct pci_capability_header {
        uint8_t cap_id;
        uint8_t next_ptr;
    }
    struct payload {
        ...
    }
}
 * The caller must provide the cap id, payload and its length.
 */
bool pci_register_device_capability(pci_dev_handle_t pci_dev_handle, uint8_t cap_id, void *payload,
                                    uint8_t payload_size);

/* Callback for virtual PCI device's BAR access. If the guest is reading, the callee is expected to write the result to `data`.
 * If the guest is writing, `data` contain the value. */
typedef bool (*pci_bar_mmio_fault_handler_t)(pci_dev_handle_t pci_dev_handle, uint64_t bar_offset, bool is_read,
                                             int access_width_bytes, uint64_t *data, void *cookie);

/* Allocate a memory BAR in the virtual PCI bus' MMIO aperature for the given PCI device. */
bool pci_register_device_mmio_bar(pci_dev_handle_t pci_dev_handle, uint8_t bar_index, uint64_t size,
                                  pci_bar_mmio_fault_handler_t callback, void *cookie);
