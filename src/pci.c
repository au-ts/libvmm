/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <libvmm/libvmm.h>

#define LOG_PCI_INFO(...) do{ printf("%s|%s INFO: ", microkit_name, __func__); printf(__VA_ARGS__); }while(0)
#define LOG_PCI_ERR(...) do{ printf("%s|%s ERROR: ", microkit_name, __func__); printf(__VA_ARGS__); }while(0)

/* Document referenced: https://wiki.osdev.org/PCI */

#define PCI_INVALID_VENDOR_ID 0xffff

/* Should be plenty for now. */
#define PCI_NUM_BUS 1
/* These are based on the x86 Configuration Space Access Mechanism #1.
 * Where there are 5 bits for the device number and 3 bits for the function number. */
#define PCI_DEV_PER_BUS 32
#define PCI_FUNC_PER_DEV 7

#define PCI_NUM_CONFIG_SPACES (PCI_NUM_BUS * PCI_DEV_PER_BUS * PCI_FUNC_PER_DEV)
/* Under the modern ECAM regime each configuration space is 4K large. */
#define PCI_CONFIG_SPACE_SIZE 0x1000
#define PCI_EXPECTED_ECAM_SIZE (PCI_NUM_CONFIG_SPACES * PCI_CONFIG_SPACE_SIZE)

/* This is hard define in the PCI config space */
#define PCI_NUM_BARS_PER_CONFIG_SPACE 6

struct pci_capability_header {
    uint8_t cap_id;
    uint8_t next_ptr;
};

// Type 0 headers for endpoints
struct pci_config_space {
    // Device Identification
    uint16_t vendor_id;           // 0x00: Vendor ID
    uint16_t device_id;           // 0x02: Device ID
    uint16_t command;             // 0x04: Command Register
    uint16_t status;              // 0x06: Status Register
    uint8_t revision_id;         // 0x08: Revision ID
    uint8_t prog_if;             // 0x09: Programming Interface
    uint8_t subclass;            // 0x0A: Sub Class Code
    uint8_t class_code;          // 0x0B: Base Class Code
    uint8_t cache_line_size;     // 0x0C: Cache Line Size
    uint8_t latency_timer;       // 0x0D: Latency Timer
    uint8_t header_type;         // 0x0E: Header Type
    uint8_t bist;                // 0x0F: Built-in Self Test

    // Base Address Registers (BARs)
    uint32_t bar[PCI_NUM_BARS_PER_CONFIG_SPACE]; // 0x10-0x27: Base Address Registers

    // Subsystem Information
    uint32_t cardbus_cis_ptr;     // 0x28: CardBus CIS Pointer
    uint16_t subsystem_vendor_id; // 0x2C: Subsystem Vendor ID
    uint16_t subsystem_device_id; // 0x2E: Subsystem Device ID
    uint32_t expansion_rom_addr;  // 0x30: Expansion ROM Base Address

    // Capabilities and Interrupts
    uint8_t cap_ptr;             // 0x34: Capabilities Pointer
    uint8_t reserved1[3];        // 0x35-0x37: Reserved
    uint32_t reserved2;           // 0x38-0x3B: Reserved
    uint8_t interrupt_line;      // 0x3C: Interrupt Line
    uint8_t interrupt_pin;       // 0x3D: Interrupt Pin
    uint8_t min_gnt;             // 0x3E: Min_Gnt
    uint8_t max_lat;             // 0x3F: Max_Lat

    // Capability list
    uint8_t cap_data[192];       // 0x40
} __attribute((packed));

_Static_assert(sizeof(struct pci_config_space) == 256, "PCI Config Space size");

struct pci_device_bar {
    bool valid;
    uint64_t gpa;
    uint64_t size;
    pci_bar_mmio_fault_handler_t callback;
    void *cookie;
};

struct pci_device {
    struct pci_config_space config_space;
    bool virq_registered;
    struct pci_device_bar bars[PCI_NUM_BARS_PER_CONFIG_SPACE];
    /* A simple bump allocator for the capabilities linked list. */
    uint8_t next_available_cap_ptr;
};

struct pci_ecam {
    uint64_t gpa;
    uint32_t size;

    /* Storage for the PCI devices' virtual configuration spaces and associated metadata.
     * Note that this is a flattened array.*/
    struct pci_device devices[PCI_NUM_CONFIG_SPACES];
};

struct pci_mmio_aperature {
    uint64_t gpa;
    uint64_t size;
    /* A bump allocator for BARs in this MMIO aperature, watermark in terms of GPA.
     * This is problematic if the guest shuffle the BARs around. */
    uint64_t watermark;
};

struct pci_bus {
    bool initialised;
    struct pci_ecam ecam;
    struct pci_mmio_aperature mmio_aperature;
};

static struct pci_bus pci_bus;

// Translate PCI Geographic Address, represented as Bus:Device.Function, to an index to `config_spaces`.
static uint32_t pci_geo_addr_to_internal_index(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (bus * PCI_DEV_PER_BUS) + (dev * PCI_FUNC_PER_DEV) + func;
}

static struct pci_device *pci_get_device(pci_dev_handle_t pci_dev_handle)
{
    return &pci_bus.ecam.devices[pci_dev_handle];
}

static bool pci_ecam_fault_handle(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data)
{
    return true;
}

static bool pci_mmio_fault_handle(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data)
{
    return true;
}

static inline bool pci_device_exist_check(pci_dev_handle_t pci_dev_handle)
{
    if (pci_dev_handle >= PCI_NUM_CONFIG_SPACES) {
        LOG_PCI_ERR("PCI handle %d is out of bound.\n", pci_dev_handle);
        return false;
    }

    struct pci_config_space *config_space = &pci_get_device(pci_dev_handle)->config_space;
    if (config_space->vendor_id != PCI_INVALID_VENDOR_ID) {
        LOG_PCI_ERR("PCI handle %d is invalid.\n", pci_dev_handle);
        return false;
    }
    return true;
}

pci_dev_handle_t pci_register_device(uint8_t bus, uint8_t dev, uint8_t func, pci_device_register_data_t *device_data)
{
    if (bus >= PCI_NUM_BUS) {
        LOG_PCI_ERR("PCI bus %u is out of bound\n", bus);
        return INVALID_PCI_DEVICE_HANDLE;
    }

    if (dev >= PCI_DEV_PER_BUS) {
        LOG_PCI_ERR("PCI dev %u is out of bound\n", bus);
        return INVALID_PCI_DEVICE_HANDLE;
    }

    if (func >= PCI_FUNC_PER_DEV) {
        LOG_PCI_ERR("PCI func %u is out of bound\n", bus);
        return INVALID_PCI_DEVICE_HANDLE;
    }

    pci_dev_handle_t handle = pci_geo_addr_to_internal_index(bus, dev, func);
    struct pci_device *pci_device = pci_get_device(handle);
    struct pci_config_space *config_space = &pci_device->config_space;
    if (config_space->vendor_id != PCI_INVALID_VENDOR_ID) {
        LOG_PCI_ERR(
            "Geo addr %u:%u.%u has already been occupied by PCI device vendor id 0x%x, device id 0x%x, handle %d\n",
            bus, dev, func, config_space->vendor_id, config_space->device_id, handle);
        return INVALID_PCI_DEVICE_HANDLE;
    }

    if (device_data->vendor_id == PCI_INVALID_VENDOR_ID) {
        LOG_PCI_ERR("Vendor ID cannot be an invalid value\n");
        return INVALID_PCI_DEVICE_HANDLE;
    }

    /* Let be safe even though we don't support deregistering a PCI device. */
    memset(pci_device, 0, sizeof(struct pci_device));

    config_space->vendor_id = device_data->vendor_id;
    config_space->device_id = device_data->device_id;
    config_space->command = device_data->command;
    config_space->status = device_data->status;
    config_space->revision_id = device_data->revision_id;
    config_space->subclass = device_data->subclass;
    config_space->class_code = device_data->class_code;
    config_space->subsystem_vendor_id = device_data->subsystem_vendor_id;
    config_space->subsystem_device_id = device_data->subsystem_device_id;

    /* Always use dev's first interrupt pin specified in interrupt-map */
    config_space->interrupt_pin = 0x1;

    pci_device->next_available_cap_ptr = offsetof(struct pci_config_space, cap_data);
    pci_device->virq_registered = false;

    return handle;
}

bool pci_register_device_irq(pci_dev_handle_t pci_dev_handle, int virq, virq_ack_fn_t ack_fn, void *ack_data)
{
    if (!pci_device_exist_check(pci_dev_handle)) {
        return false;
    }

    struct pci_device *pci_device = pci_get_device(pci_dev_handle);
    if (pci_device->virq_registered) {
        LOG_PCI_ERR("IRQ already registered for PCI dev handle %d\n", pci_dev_handle);
        return false;
    }

    bool success = virq_register(GUEST_BOOT_VCPU_ID, virq, ack_fn, ack_data);
    if (pci_device->virq_registered) {
        LOG_PCI_ERR("Cannot register IRQ for PCI dev handle %d\n", pci_dev_handle);
        return false;
    }

    pci_device->virq_registered = success;
    return success;
}

bool pci_register_device_capability(pci_dev_handle_t pci_dev_handle, uint8_t cap_id, void *payload,
                                    uint8_t payload_size)
{
    if (!pci_device_exist_check(pci_dev_handle)) {
        return false;
    }

    struct pci_device *pci_device = pci_get_device(pci_dev_handle);
    struct pci_config_space *config_space = &pci_device->config_space;

    uint16_t remaining_space = sizeof(struct pci_config_space) - pci_device->next_available_cap_ptr;
    uint16_t cap_size = payload_size + sizeof(struct pci_capability_header);
    if (cap_size > remaining_space) {
        LOG_PCI_ERR("Out of space for registering cap id %u, size %u, for PCI dev handle %d\n", cap_id, cap_size,
                    pci_dev_handle);
        return false;
    }

    /* Walk to the last cap and update the next ptr. If there is no existing cap then the logic is still ok
     * since we will overwrite the next ptr at the last step. */
    {
        struct pci_capability_header *curr_cap = (struct pci_capability_header *)((uintptr_t)config_space
                                                                                  + config_space->cap_ptr);
        assert(config_space->cap_ptr == offsetof(struct pci_config_space, cap_data));
        while (curr_cap->next_ptr) {
            curr_cap = (struct pci_capability_header *)((uintptr_t)config_space + curr_cap->next_ptr);
        }
        assert(curr_cap->next_ptr == 0);

        curr_cap->next_ptr = pci_device->next_available_cap_ptr;
    }

    assert(((uint16_t)pci_device->next_available_cap_ptr) + cap_size <= sizeof(struct pci_config_space));
    pci_device->next_available_cap_ptr += cap_size;

    struct pci_capability_header *dest = (struct pci_capability_header *)(((char *)config_space)
                                                                          + pci_device->next_available_cap_ptr);
    dest->cap_id = cap_id;
    dest->next_ptr = 0;
    memcpy((void *)(((char *)dest) + sizeof(struct pci_capability_header)), payload, payload_size);

    return true;
}

bool pci_register_device_mmio_bar(pci_dev_handle_t pci_dev_handle, uint8_t bar_index, uint64_t size,
                                  pci_bar_mmio_fault_handler_t callback, void *cookie)
{
    if (!pci_device_exist_check(pci_dev_handle)) {
        return false;
    }

    if (bar_index >= PCI_NUM_BARS_PER_CONFIG_SPACE) {
        LOG_PCI_ERR("bar index %u is out of bound\n", bar_index);
        return false;
    }

    struct pci_device *pci_device = pci_get_device(pci_dev_handle);
    if (pci_device->bars[bar_index].valid) {
        LOG_PCI_ERR("bar index %u is is already registered for PCI device handle %d\n", bar_index, pci_dev_handle);
        return false;
    }

    uint64_t space_remaining_in_aperature = pci_bus.mmio_aperature.gpa + pci_bus.mmio_aperature.size
                                          - pci_bus.mmio_aperature.watermark;
    if (space_remaining_in_aperature < size) {
        LOG_PCI_ERR("out of space in MMIO aperature to register bar index %u, size %lu for PCI device handle %d\n",
                    bar_index, size, pci_dev_handle);
        return false;
    }

    pci_device->bars[bar_index].gpa = pci_bus.mmio_aperature.watermark;
    // @billn todo contine MMIO bar allocation logic, malipulate watermark, register VM fault handlers etc
    // create a generic fault handler for registration, give the device struct as the cookie
}

bool pci_bus_init(uint64_t ecam_gpa, uint32_t ecam_size, uint64_t mmio_aperature_gpa, uint64_t mmio_aperature_size)
{
    if (ecam_size != PCI_EXPECTED_ECAM_SIZE) {
        LOG_PCI_ERR("`ecam_size` = 0x%x does not equal expected 0x%x!\n", ecam_size, PCI_EXPECTED_ECAM_SIZE);
        return false;
    }

    if (mmio_aperature_size == 0) {
        LOG_PCI_ERR("mmio_aperature_size cannot be zero.\n");
        return false;
    }

    memset(&pci_bus, 0, sizeof(struct pci_bus));

    pci_bus.ecam.gpa = ecam_gpa;
    pci_bus.ecam.size = ecam_size;
    pci_bus.mmio_aperature.gpa = mmio_aperature_gpa;
    pci_bus.mmio_aperature.size = mmio_aperature_size;

    for (int i = 0; i < PCI_NUM_CONFIG_SPACES; i++) {
        pci_bus.ecam.devices[i].config_space.vendor_id = PCI_INVALID_VENDOR_ID;
    }

    if (!fault_register_vm_exception_handler(ecam_gpa, ecam_size, &pci_ecam_fault_handle, NULL)) {
        LOG_PCI_ERR("Could not register virtual memory fault handler for PCI ECAM area!\n");
        return false;
    }

    /* We don't register fault handlers for the MMIO aperature yet as they will be individually handled later. */

    pci_bus.initialised = true;

    return true;
}