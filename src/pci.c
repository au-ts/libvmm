/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <libvmm/libvmm.h>

#if defined(CONFIG_ARCH_X86)
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/pci.h>
#endif

#define LOG_PCI_INFO(...) do{ printf("%s|%s INFO: ", microkit_name, __func__); printf(__VA_ARGS__); }while(0)
#define LOG_PCI_ERR(...) do{ printf("%s|%s ERROR: ", microkit_name, __func__); printf(__VA_ARGS__); }while(0)

/* Document referenced: https://wiki.osdev.org/PCI */

#define PCI_INVALID_VENDOR_ID 0xffff

/* Should be plenty for now. */
#define PCI_NUM_BUS 1
/* These are based on the x86 Configuration Space Access Mechanism #1.
 * Where there are 5 bits for the device number and 3 bits for the function number. */
#define PCI_DEV_PER_BUS 32
#define PCI_FUNC_PER_DEV 8

#define PCI_NUM_CONFIG_SPACES (PCI_NUM_BUS * PCI_DEV_PER_BUS * PCI_FUNC_PER_DEV)
/* Under the modern ECAM regime each configuration space is 4K large. */
#define PCI_CONFIG_SPACE_SIZE 0x1000
#define PCI_EXPECTED_ECAM_SIZE (PCI_NUM_CONFIG_SPACES * PCI_CONFIG_SPACE_SIZE)

/* This is hard define in the PCI config space */
#define PCI_NUM_BARS_PER_CONFIG_SPACE 6

/* PCI Status Register */
#define PCI_STATUS_IMM_READY            0x01    /* Immediate Readiness */
#define PCI_STATUS_INTERRUPT            0x08    /* Interrupt status */
#define PCI_STATUS_CAP_LIST             0x10    /* Support Capability List */

#define PCI_CFG_OFFSET_COMMAND       0x04
#define PCI_CFG_OFFSET_STATUS        0x06
#define PCI_CFG_OFFSET_BAR1          0x10
#define PCI_CFG_OFFSET_BAR2          0x14
#define PCI_CFG_OFFSET_BAR3          0x18
#define PCI_CFG_OFFSET_BAR4          0x1C
#define PCI_CFG_OFFSET_BAR5          0x20
#define PCI_CFG_OFFSET_BAR6          0x24

#define PCI_HEADER_TYPE_MULTIFUNCTION 0x80

struct pci_bar_memory_bits {
    uint32_t memory_type : 1;    // Bit 0: 0 = Memory space
    uint32_t mem_type : 2;    // Bits 2-1: Memory type
    uint32_t prefetchable : 1;    // Bit 3: Prefetchable
    uint32_t base_address : 28;   // Bits 31-4: Base address
} __attribute__((packed));

_Static_assert(sizeof(struct pci_bar_memory_bits) == 4, "struct pci_bar_memory_bits size != 32 bits");

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
    struct pci_bar_memory_bits bars[PCI_NUM_BARS_PER_CONFIG_SPACE]; // 0x10-0x27: Base Address Registers

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
    uint32_t sticky_cmd_bits;
    struct pci_config_space config_space;
    bool virq_registered;
    irq_routing_info_t irq_routing_info;
    /* NOTE that 64 bit BARs consume 2 slots! */
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
};

struct pci_bus {
    bool initialised;
    struct pci_ecam ecam;
    struct pci_mmio_aperature mmio_aperature;

#if defined(CONFIG_ARCH_X86)
    uint32_t pio_addr_value;
#endif
};

static struct pci_bus pci_bus;

// Translate PCI Geographic Address, represented as Bus:Device.Function, to an index to `config_spaces`.
static pci_dev_handle_t pci_geo_addr_to_internal_index(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (pci_dev_handle_t)((bus * PCI_DEV_PER_BUS) + (dev * PCI_FUNC_PER_DEV) + func);
}

static struct pci_device *pci_get_device(pci_dev_handle_t pci_dev_handle)
{
    return &pci_bus.ecam.devices[pci_dev_handle];
}

static inline bool pci_device_exist_check(pci_dev_handle_t pci_dev_handle)
{
    if (pci_dev_handle < 0) {
        LOG_PCI_ERR("PCI handle %d is invalid\n", pci_dev_handle);
        return false;
    }

    if (pci_dev_handle >= PCI_NUM_CONFIG_SPACES) {
        LOG_PCI_ERR("PCI handle %d is out of bound.\n", pci_dev_handle);
        return false;
    }

    struct pci_config_space *config_space = &pci_get_device(pci_dev_handle)->config_space;
    if (config_space->vendor_id == PCI_INVALID_VENDOR_ID) {
        LOG_PCI_ERR("PCI handle %d is invalid.\n", pci_dev_handle);
        return false;
    }
    return true;
}

static inline bool pci_bus_initialised_check(void)
{
    if (!pci_bus.initialised) {
        LOG_PCI_ERR("PCI bus not initialised!\n");
    }
    return pci_bus.initialised;
}

#if defined(CONFIG_ARCH_ARM)
static bool pci_bar_fault_handler(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *cookie)
#elif defined(CONFIG_ARCH_X86)
static bool pci_bar_fault_handler(size_t vcpu_id, size_t offset, size_t qualification,
                                  decoded_instruction_ret_t decoded_ins, seL4_VCPUContext *vctx, void *cookie)
#endif
{
    pci_dev_handle_t handle = (pci_dev_handle_t)(((uint64_t)(cookie)) << 3) >> 3;
    if (!pci_device_exist_check(handle)) {
        return false;
    }

    struct pci_device *pci_device = pci_get_device(handle);
    uint8_t bar_idx = ((uint64_t)cookie) >> 61;
    assert(bar_idx < PCI_NUM_BARS_PER_CONFIG_SPACE);

    bool is_read;
    int access_width_bytes;
    uint64_t data = 0;

#if defined(CONFIG_ARCH_ARM)
    is_read = fault_is_read(fsr);
    access_width_bytes = fault_get_width_bytes(fsr);
#elif defined(CONFIG_ARCH_X86)
    is_read = ept_fault_is_read(qualification);
    access_width_bytes = mem_access_width_to_bytes(decoded_ins);
#endif

    if (!is_read) {
#if defined(CONFIG_ARCH_ARM)
        data = fault_get_data(regs, fsr);
#elif defined(CONFIG_ARCH_X86)
        assert(mem_write_get_data(decoded_ins, qualification, vctx, &data));
#endif
    }

    bool success = pci_device->bars[bar_idx].callback(handle, offset, is_read, access_width_bytes, &data,
                                                      pci_device->bars[bar_idx].cookie);

    if (success && is_read) {
#if defined(CONFIG_ARCH_ARM)
        fault_emulate_write(regs, offset, fsr, data);
#elif defined(CONFIG_ARCH_X86)
        assert(mem_read_set_data(decoded_ins, qualification, vctx, offset, data));
#endif
    }

    return success;
}

static bool pci_ecam_emulate_access(pci_dev_handle_t handle, bool is_read, int access_width_bytes,
                                    int config_space_offset, uint64_t *data)
{
    struct pci_device *pci_device = pci_get_device(handle);
    struct pci_config_space *config_space = &pci_device->config_space;

    if (is_read) {
        /* The PCI config space is in 32-bit words, so just read it like this, then let the arch
         * specific layer perform the shifting. */
        uint32_t *reg = (uint32_t *)config_space;
        *data = (uint32_t)reg[config_space_offset / 4];
    } else {
        switch (config_space_offset) {
        case REG_RANGE(PCI_CFG_OFFSET_COMMAND, PCI_CFG_OFFSET_STATUS): {
            config_space->command = (uint16_t)*data & pci_device->sticky_cmd_bits;
            break;
        }
        case REG_RANGE(PCI_CFG_OFFSET_BAR1, PCI_CFG_OFFSET_BAR2): {
            uint8_t dev_bar_id = (config_space_offset - PCI_CFG_OFFSET_BAR1) / 0x4;
            assert(dev_bar_id < PCI_NUM_BARS_PER_CONFIG_SPACE);

            // @billn handle 64-bit BARs
            // Memory negotiation process:
            //     1. The driver writes all 1s to the BAR register.
            //     2. The device writes the size mask ~(size - 1) to the BAR register.
            //     3. The driver writes the original value (all 0s in our code) back
            //         to the BAR register. (no action from the device)
            //     4. The driver allocates memory from the memory resources, and writes
            //         the allocated address to the BAR register.
            //     5. The device parse the memory address and bookkeep it.
            if (pci_device->bars[dev_bar_id].size) {
                if (*data == 0xFFFFFFFF) {
                    struct pci_bar_memory_bits *bar = &config_space->bars[dev_bar_id];
                    bar->base_address = (~(pci_device->bars[dev_bar_id].size - 1)) >> 4;
                } else if (*data != 0x0) {
                    struct pci_bar_memory_bits *bar = &config_space->bars[dev_bar_id];
                    uint32_t guest_allocated_addr = *data & 0xFFFFFFF0;   // Ignore control bits
                    bar->base_address = guest_allocated_addr >> 4;       // 16-byte aligned
                    pci_device->bars[dev_bar_id].gpa = guest_allocated_addr;

                    /* We just need 3 bits to represent the BAR index (0-5), so we can stuff it
                     * to the top bits of the handle. */
                    uint64_t cookie = handle;
                    /* Make sure the top 3 bits aren't used. */
                    assert((cookie & (BIT(63) | BIT(62) | BIT(61))) == 0);
                    uint64_t bar_idx_mask = (uint64_t)dev_bar_id << 61;
                    cookie |= bar_idx_mask;

                    bool register_ok;
#if defined(CONFIG_ARCH_ARM)
                    register_ok = fault_register_vm_exception_handler(guest_allocated_addr,
                                                                      pci_device->bars[dev_bar_id].size,
                                                                      &pci_bar_fault_handler, (void *)cookie);
#elif defined(CONFIG_ARCH_X86)
                    register_ok = fault_register_ept_exception_handler(guest_allocated_addr,
                                                                       pci_device->bars[dev_bar_id].size,
                                                                       &pci_bar_fault_handler, (void *)cookie);
#endif
                    if (!register_ok) {

                        LOG_VMM_ERR("Could not register MMIO fault handler for BAR %d of PCI device handle %d\n",
                                    dev_bar_id, handle);
                        return false;
                    }
                }
            }
            break;
        }
        }
    }
    return true;
}

#if defined(CONFIG_ARCH_ARM)
static bool pci_ecam_memory_fault_handle(size_t vcpu_id, size_t ecam_offset, size_t fsr, seL4_UserContext *regs,
                                         void *cookie)
#elif defined(CONFIG_ARCH_X86)
static bool pci_ecam_memory_fault_handle(size_t qualification, size_t ecam_offset,
                                         decoded_instruction_ret_t decoded_ins, seL4_VCPUContext *vctx, void *cookie)
#endif
{
    uint16_t bus = ecam_offset >> 20;
    uint16_t dev = (ecam_offset >> 15) & 0x1F;
    uint16_t func = (ecam_offset >> 12) & 0x7;
    uint16_t offset = ecam_offset & 0xFF; /* The mask needs to be updated if we support the extended config space. */

    if (bus >= PCI_NUM_BUS) {
        return false;
    }
    if (dev >= PCI_DEV_PER_BUS) {
        return false;
    }
    if (func >= PCI_FUNC_PER_DEV) {
        return false;
    }

    pci_dev_handle_t handle = pci_geo_addr_to_internal_index(bus, dev, func);
    /* We don't need to check if the device is registered, because in that case we already initialised
     * vendor ID to PCI_INVALID_VENDOR_ID. */

    bool is_read;
    int access_width_bytes;
    uint64_t data = 0;

#if defined(CONFIG_ARCH_ARM)
    is_read = fault_is_read(fsr);
    access_width_bytes = fault_get_width_bytes(fsr);
#elif defined(CONFIG_ARCH_X86)
    is_read = ept_fault_is_read(qualification);
    access_width_bytes = mem_access_width_to_bytes(decoded_ins);
#endif

    if (!is_read) {
#if defined(CONFIG_ARCH_ARM)
        data = fault_get_data(regs, fsr);
#elif defined(CONFIG_ARCH_X86)
        assert(mem_write_get_data(decoded_ins, qualification, vctx, &data));
#endif
    }

    bool success = pci_ecam_emulate_access(handle, is_read, access_width_bytes, offset, &data);

    if (success && is_read) {
#if defined(CONFIG_ARCH_ARM)
        fault_emulate_write(regs, offset, fsr, data);
#elif defined(CONFIG_ARCH_X86)
        assert(mem_read_set_data(decoded_ins, qualification, vctx, ecam_offset, data));
#endif
    }

    return success;
}

#if defined(CONFIG_ARCH_X86)
static bool pci_pio_select_fault_handle(size_t vcpu_id, uint16_t port_offset, size_t qualification,
                                        seL4_VCPUContext *vctx, void *cookie)
{
    if (pio_fault_is_read(qualification)) {
        assert(pio_fault_addr(qualification) == PCI_CONFIG_ADDRESS_START_PORT);
        vctx->eax = pci_bus.pio_addr_value;
    } else {
        pci_bus.pio_addr_value = vctx->eax;
    }

    return true;
}

static bool pci_pio_data_fault_handle(size_t vcpu_id, uint16_t port_offset, size_t qualification,
                                      seL4_VCPUContext *vctx, void *cookie)
{
    assert(!pio_fault_is_string_op(qualification));

    if (!pci_pio_addr_reg_enable(pci_bus.pio_addr_value)) {
        emulate_ioport_noop_access(vctx, qualification);
        return true;
    }

    uint8_t bus = pci_pio_addr_reg_bus(pci_bus.pio_addr_value);
    uint8_t dev = pci_pio_addr_reg_dev(pci_bus.pio_addr_value);
    uint8_t func = pci_pio_addr_reg_func(pci_bus.pio_addr_value);
    if (bus >= PCI_NUM_BUS) {
        emulate_ioport_noop_access(vctx, qualification);
        return true;
    }
    if (dev >= PCI_DEV_PER_BUS) {
        emulate_ioport_noop_access(vctx, qualification);
        return true;
    }
    if (func >= PCI_FUNC_PER_DEV) {
        emulate_ioport_noop_access(vctx, qualification);
        return true;
    }

    uint8_t config_space_off = pci_pio_addr_reg_offset(pci_bus.pio_addr_value) + port_offset;
    int access_width_bytes = pio_fault_to_access_width_bytes(qualification);

    /* Let's make sure that the access does not cross a 32 bits boundary. */
    if (config_space_off % 4) {
        if (config_space_off + access_width_bytes > ROUND_UP(config_space_off, 4)) {
            LOG_PCI_ERR("access crosses 32 bit boundary!\n");
        }
    }

    bool success;
    pci_dev_handle_t handle = pci_geo_addr_to_internal_index(bus, dev, func);
    if (pio_fault_is_read(qualification)) {
        success = pci_ecam_emulate_access(handle, pio_fault_is_read(qualification), access_width_bytes,
                                          config_space_off, &vctx->eax);

        vctx->eax >>= (config_space_off - ROUND_DOWN(config_space_off, 4)) * 8;

        if (access_width_bytes == 1) {
            vctx->eax &= 0xff;
        } else if (access_width_bytes == 2) {
            vctx->eax &= 0xffff;
        }
    } else {
        success = pci_ecam_emulate_access(handle, pio_fault_is_read(qualification), access_width_bytes,
                                          config_space_off, &vctx->eax);
    }

    return success;
}
#endif

pci_dev_handle_t pci_register_device(uint8_t bus, uint8_t dev, uint8_t func, pci_device_register_data_t *device_data)
{
    if (!pci_bus_initialised_check()) {
        return false;
    }

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

    pci_device->next_available_cap_ptr = offsetof(struct pci_config_space, cap_data);
    pci_device->virq_registered = false;

    pci_device->sticky_cmd_bits = device_data->command;

    /* Set the multifunction bit if applicable. */
    if (func > 0) {
        for (int f = 0; f < PCI_FUNC_PER_DEV; f++) {
            handle = pci_geo_addr_to_internal_index(bus, dev, f);
            pci_device = pci_get_device(handle);
            config_space = &pci_device->config_space;

            if (config_space->vendor_id != PCI_INVALID_VENDOR_ID) {
                config_space->header_type = PCI_HEADER_TYPE_MULTIFUNCTION;
            }
        }
    }

    return handle;
}

bool pci_register_device_irq(pci_dev_handle_t pci_dev_handle, irq_routing_info_t irq_routing_info, virq_ack_fn_t ack_fn,
                             void *ack_data)
{
    if (!pci_bus_initialised_check()) {
        return false;
    }

    if (!pci_device_exist_check(pci_dev_handle)) {
        return false;
    }

    struct pci_device *pci_device = pci_get_device(pci_dev_handle);
    if (pci_device->virq_registered) {
        LOG_PCI_ERR("IRQ already registered for PCI dev handle %d\n", pci_dev_handle);
        return false;
    }

    bool success = virq_register(irq_routing_info, ack_fn, ack_data);
    if (!success) {
        LOG_PCI_ERR("Cannot register IRQ for PCI dev handle %d\n", pci_dev_handle);
        return false;
    }

    /* Always use dev's first interrupt pin specified in interrupt-map (INTA) */
    pci_device->irq_routing_info = irq_routing_info;
    pci_device->config_space.interrupt_pin = 0x1;

    switch (irq_routing_info.type) {
    case IRQ_TYPE_ARM_GIC:
        pci_device->config_space.interrupt_line = irq_routing_info.hw.arm_gic.intid;
        break;
    case IRQ_TYPE_X86_IOAPIC:
        /* @billn revisit for multiple I/O APIC and MSI */
        pci_device->config_space.interrupt_line = irq_routing_info.hw.x86_ioapic.pin;
        break;
    default:
        assert(0);
    }

    pci_device->virq_registered = success;
    return success;
}

bool pci_device_set_irq_status(pci_dev_handle_t pci_dev_handle, bool new_status)
{
    if (!pci_bus_initialised_check()) {
        return false;
    }

    if (!pci_device_exist_check(pci_dev_handle)) {
        return false;
    }

    struct pci_device *pci_device = pci_get_device(pci_dev_handle);
    if (!pci_device->virq_registered) {
        LOG_PCI_ERR("IRQ not registered PCI dev handle %d\n", pci_dev_handle);
        return false;
    }

    if (new_status) {
        pci_device->config_space.status |= PCI_STATUS_INTERRUPT;
    } else {
        pci_device->config_space.status &= ~PCI_STATUS_INTERRUPT;
    }

    return true;
}

bool pci_register_device_capability(pci_dev_handle_t pci_dev_handle, uint8_t cap_id, void *payload,
                                    uint8_t payload_size)
{
    if (!pci_bus_initialised_check()) {
        return false;
    }

    if (!pci_device_exist_check(pci_dev_handle)) {
        return false;
    }

    struct pci_device *pci_device = pci_get_device(pci_dev_handle);
    struct pci_config_space *config_space = &pci_device->config_space;

    uint16_t cap_size = payload_size + sizeof(struct pci_capability_header);
    if ((uint16_t)pci_device->next_available_cap_ptr + cap_size > sizeof(struct pci_config_space)) {
        LOG_PCI_ERR("Out of space for registering cap id %u, size %u, for PCI dev handle %d\n", cap_id, cap_size,
                    pci_dev_handle);
        return false;
    }

    if (config_space->cap_ptr == 0) {
        /* First cap */
        config_space->cap_ptr = pci_device->next_available_cap_ptr;
        config_space->status |= PCI_STATUS_CAP_LIST;
    } else {
        /* Walk to the last cap and update the next ptr. */
        struct pci_capability_header *curr_cap = (struct pci_capability_header *)((uintptr_t)config_space
                                                                                  + config_space->cap_ptr);
        while (curr_cap->next_ptr) {
            curr_cap = (struct pci_capability_header *)((uintptr_t)config_space + curr_cap->next_ptr);
        }
        curr_cap->next_ptr = pci_device->next_available_cap_ptr;
    }

    struct pci_capability_header *dest = (struct pci_capability_header *)(((char *)config_space)
                                                                          + pci_device->next_available_cap_ptr);
    assert(((uint16_t)pci_device->next_available_cap_ptr) + cap_size <= sizeof(struct pci_config_space));

    dest->cap_id = cap_id;
    dest->next_ptr = 0;
    memcpy((void *)(((char *)dest) + sizeof(struct pci_capability_header)), payload, payload_size);

    pci_device->next_available_cap_ptr = ROUND_UP(pci_device->next_available_cap_ptr + cap_size, 4);

    return true;
}

bool pci_register_device_mmio_bar(pci_dev_handle_t pci_dev_handle, uint8_t bar_index, uint64_t size,
                                  pci_bar_mmio_fault_handler_t callback, void *cookie)
{
    if (!pci_bus_initialised_check()) {
        return false;
    }

    if (!pci_device_exist_check(pci_dev_handle)) {
        return false;
    }

    if (!size) {
        LOG_PCI_ERR("bar size is zero\n");
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

    pci_device->bars[bar_index].valid = true;
    pci_device->bars[bar_index].size = size;
    pci_device->bars[bar_index].callback = callback;
    pci_device->bars[bar_index].cookie = cookie;

    return true;
}

bool pci_bus_init(uint64_t ecam_gpa, uint32_t ecam_size, uint64_t mmio_aperature_gpa, uint64_t mmio_aperature_size)
{
    /* x86 does not need ECAM for PCI */
#if !defined(CONFIG_ARCH_X86)
    if (ecam_size != PCI_EXPECTED_ECAM_SIZE) {
        LOG_PCI_ERR("`ecam_size` = 0x%x does not equal expected 0x%x!\n", ecam_size, PCI_EXPECTED_ECAM_SIZE);
        return false;
    }
#endif

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

#if defined(CONFIG_ARCH_ARM)
    if (!fault_register_vm_exception_handler(ecam_gpa, ecam_size, &pci_ecam_memory_fault_handle, NULL)) {
        LOG_PCI_ERR("Could not register virtual memory fault handler for PCI ECAM area!\n");
        return false;
    }
#elif defined(CONFIG_ARCH_X86)
    /* Don't care about ECAM on x86 for now, just do the PIO access mechanism #1 */
    if (!fault_register_pio_exception_handler(PCI_CONFIG_ADDRESS_START_PORT, PCI_CONFIG_ADDRESS_PORT_SIZE,
                                              pci_pio_select_fault_handle, NULL)) {
        LOG_PCI_ERR("Could not register PIO fault handler for PCI mech #1 select register!\n");
        return false;
    }
    if (!fault_register_pio_exception_handler(PCI_CONFIG_DATA_START_PORT, PCI_CONFIG_DATA_PORT_SIZE,
                                              pci_pio_data_fault_handle, NULL)) {
        LOG_PCI_ERR("Could not register PIO fault handler for PCI mech #1 data register!\n");
        return false;
    }
#endif

    /* We don't register fault handlers for the MMIO aperature yet as they will be individually handled later. */

    pci_bus.initialised = true;

    return true;
}