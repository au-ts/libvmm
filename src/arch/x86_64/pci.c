/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <string.h>
#include <libvmm/util/util.h>
#include <libvmm/guest.h>
#include <sddf/util/util.h>
#include <libvmm/arch/x86_64/pci.h>
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/memory_space.h>
#include <libvmm/virtio/pci.h>

/* Uncomment this to enable debug logging */
// #define DEBUG_PCI_PIO

#if defined(DEBUG_PCI_PIO)
#define LOG_PCI_PIO(...) do{ printf("%s|PCI PIO: ", microkit_name); printf(__VA_ARGS__); }while(0)
#else
#define LOG_PCI_PIO(...) do{}while(0)
#endif

// #define DEBUG_PCI_ECAM

#if defined(DEBUG_PCI_ECAM)
#define LOG_PCI_ECAM(...) do{ printf("%s|PCI ECAM: ", microkit_name); printf(__VA_ARGS__); }while(0)
#else
#define LOG_PCI_ECAM(...) do{}while(0)
#endif

#define INTEL_82441_DEVICE_ID  0x1237

struct pci_x86_state {
    uint32_t selected_pio_addr_reg;
    // Backing storage for any virtual PCI devices' configuration space,
    // Laid out as an ECAM region, so that when we add virtio devices and UEFI support it's simpler.
    char ecam[ECAM_SIZE];
};

struct pci_x86_state pci_x86_state;

// x86 PIO access
static bool pci_pio_addr_reg_enable(void)
{
    return !!(pci_x86_state.selected_pio_addr_reg >> 31);
}

static uint8_t pci_pio_addr_reg_bus(void)
{
    return (pci_x86_state.selected_pio_addr_reg >> 16) & 0x7f;
}

static uint8_t pci_pio_addr_reg_dev(void)
{
    return (pci_x86_state.selected_pio_addr_reg >> 11) & 0x1f;
}

static uint8_t pci_pio_addr_reg_func(void)
{
    return (pci_x86_state.selected_pio_addr_reg >> 8) & 0x7;
}

static uint8_t pci_pio_addr_reg_offset(void)
{
    return pci_x86_state.selected_pio_addr_reg & 0xff;
}

static void pci_invalid_pio_read(seL4_VCPUContext *vctx)
{
    uint64_t *vctx_raw = (uint64_t *)vctx;
    vctx_raw[RAX_IDX] = ~0ull;
}

static char *pci_get_ecam(void)
{
    return (char *)&pci_x86_state.ecam;
}

// Translate PCI Geographic Address, represented as Bus:Device.Function, to an offset in the ECAM region
static uint32_t pci_geo_addr_to_ecam_offset(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (bus << 20) | ((dev & 0x1f) << 15) | ((func & 0x7) << 12);
}

static bool pci_pio_select_fault_handle(size_t vcpu_id, uint16_t port_offset, size_t qualification,
                                        seL4_VCPUContext *vctx, void *cookie)
{
    uint64_t is_read = qualification & BIT(3);
    uint16_t port_addr = (qualification >> 16) & 0xffff;

    if (is_read) {
        LOG_PCI_PIO("reg select read: 0x%x\n", pci_x86_state.selected_pio_addr_reg);
        assert(port_addr == PCI_CONFIG_ADDRESS_START_PORT);
        vctx->eax = pci_x86_state.selected_pio_addr_reg;
    } else {
        uint32_t value = vctx->eax;
        LOG_PCI_PIO("reg select write: 0x%x\n", value);

        if (value >> 31) {
            // @billn revisit
            // vcpu_print_regs(0);
            // assert(port_addr == PCI_CONFIG_ADDRESS_START_PORT);
            pci_x86_state.selected_pio_addr_reg = value;

            LOG_PCI_PIO("selecting bus %d, device %d, func %d, reg_offset 0x%x\n", pci_host_bridge_pio_addr_reg_bus(),
                        pci_host_bridge_pio_addr_reg_dev(), pci_host_bridge_pio_addr_reg_func(),
                        pci_host_bridge_pio_addr_reg_offset());
        } else {
            pci_x86_state.selected_pio_addr_reg = 0;
        }
    }

    return true;
}

static bool pci_config_space_read_access(uint8_t bus, uint8_t dev, uint8_t func, uint16_t reg_off, seL4_Word *data,
                                         int access_width_bytes)
{
    uint32_t config_space_ecam_off = pci_geo_addr_to_ecam_offset(bus, dev, func);
    uint8_t *bytes = (uint8_t *)(pci_get_ecam() + config_space_ecam_off + reg_off);
    *data = 0;
    memcpy(data, bytes, access_width_bytes);
    return true;
}

static bool pci_config_space_write_access(uint8_t bus, uint8_t dev, uint8_t func, uint16_t reg_off, uint32_t data,
                                          int access_width_bytes)
{

    uint32_t config_space_ecam_off = pci_geo_addr_to_ecam_offset(bus, dev, func);
    struct pci_config_space *config_space = (struct pci_config_space *)(pci_get_ecam() + config_space_ecam_off);

    // Make sure the guest can't clobber the common header
    if (reg_off >= 0x40) {
        uint8_t *bytes = (uint8_t *)((uintptr_t)config_space + reg_off);
        memcpy(bytes, &data, access_width_bytes);
    }

    return true;
}

static bool pci_pio_data_fault_handle(size_t vcpu_id, uint16_t port_offset, size_t qualification,
                                      seL4_VCPUContext *vctx, void *cookie)
{
    // @billn make helpers
    uint64_t is_read = qualification & BIT(3);
    // uint16_t port_addr = (qualification >> 16) & 0xffff;
    ioport_access_width_t access_width = (ioport_access_width_t)(qualification & 0x7);

    uint64_t is_string = qualification & BIT(4);
    assert(!is_string);

    if (!pci_pio_addr_reg_enable()) {
        pci_invalid_pio_read(vctx);
        return true;
    } else if (pci_pio_addr_reg_bus() > 0) {
        // the real backing ECAM only have enough space for 1 bus
        pci_invalid_pio_read(vctx);
        return true;
    } else {
        uint8_t bus = pci_pio_addr_reg_bus();
        uint8_t dev = pci_pio_addr_reg_dev();
        uint8_t func = pci_pio_addr_reg_func();
        uint8_t reg_off = pci_pio_addr_reg_offset() + port_offset;

        LOG_PCI_PIO("accessing bus %d, dev %d, func %d, reg off 0x%x, is read $d\n", bus, dev, func, reg_off, is_read);

        if (is_read) {
            return pci_config_space_read_access(bus, dev, func, reg_off, &(vctx->eax),
                                                ioports_access_width_to_bytes(access_width));
        } else {
            return pci_config_space_write_access(bus, dev, func, reg_off, vctx->eax,
                                                 ioports_access_width_to_bytes(access_width));
        }
    }
}

bool pci_ecam_add_device(uint8_t bus, uint8_t dev, uint8_t func, struct pci_config_space *config_space)
{
    uint32_t offset = pci_geo_addr_to_ecam_offset(bus, dev, func);
    struct pci_config_space *config_space_dest = (struct pci_config_space *)(pci_get_ecam() + offset);
    memcpy(config_space_dest, config_space, sizeof(struct pci_config_space));
    return true;
}

bool pci_x86_init(void)
{
    // Invalidate all configuration spaces in the ECAM
    int bus = 0;
    for (int dev = 0; dev <= 0x1f; dev++) {
        for (int func = 0; func <= 0x7; func++) {
            struct pci_config_space *config_space =
                (struct pci_config_space *)(pci_get_ecam() + pci_geo_addr_to_ecam_offset(bus, dev, func));
            config_space->vendor_id = 0xffff;
        }
    }

    // Register Port I/O handlers for configuration access mechanism #1
    bool success = fault_register_pio_exception_handler(PCI_CONFIG_ADDRESS_START_PORT, PCI_CONFIG_ADDRESS_PORT_SIZE,
                                                        pci_pio_select_fault_handle, NULL);
    if (!success) {
        LOG_VMM_ERR("Could not register PIO fault handler for PCI mech #1 select register!\n");
        return success;
    }
    success = fault_register_pio_exception_handler(PCI_CONFIG_DATA_START_PORT, PCI_CONFIG_DATA_PORT_SIZE,
                                                   pci_pio_data_fault_handle, NULL);
    if (!success) {
        LOG_VMM_ERR("Could not register PIO fault handler for PCI mech #1 data register!\n");
        return success;
    }

    // Populate the PCI bus with the host and ISA bridges
    struct pci_config_space host_bridge;
    memset(&host_bridge, 0, sizeof(struct pci_config_space));

    host_bridge = (struct pci_config_space) {
        .device_id = INTEL_82441_DEVICE_ID,
        .vendor_id = 0x8086,
        .class_code = 0x6,
        .subclass = 0,
        .header_type = 0,
        .command = 7 // device responds to PIO and MMIO access and DMA capable
    };

    struct pci_config_space isa_bridge;
    memset(&isa_bridge, 0, sizeof(struct pci_config_space));

    isa_bridge = (struct pci_config_space) { .device_id = 0x7000,
                                             .vendor_id = 0x8086,
                                             .class_code = 0x6,
                                             .subclass = 1,
                                             .header_type = 0x80, // bit 7 multifunction
                                             .command = 7 };

    assert(pci_ecam_add_device(0, 0, 0, &host_bridge));
    assert(pci_ecam_add_device(0, 1, 0, &isa_bridge));
    // Power management function
    assert(pci_ecam_add_device(0, 1, 3, &isa_bridge));

    return true;
}
