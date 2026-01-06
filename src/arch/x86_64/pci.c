/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <libvmm/guest.h>
#include <sddf/util/util.h>
#include <sddf/util/custom_libc/string.h>
#include <libvmm/arch/x86_64/pci.h>
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/virtio/pci.h>

extern uint64_t pci_conf_addr_pio_id;
extern uint64_t pci_conf_addr_pio_addr;

extern uint64_t pci_conf_data_pio_id;
extern uint64_t pci_conf_data_pio_addr;



/* Utility functions to access the host PCI bus */
uint32_t pci_compute_port_address(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off)
{
    uint32_t lbus = (uint32_t)bus;
    uint32_t ldev = (uint32_t)dev;
    uint32_t lfunc = (uint32_t)func;

    // Bit 31     | Bits 30-24 | Bits 23-16 | Bits 15-11    | Bits 10-8       | Bits 7-0
    // Enable Bit | Reserved   | Bus Number | Device Number | Function Number | Register Offset

    /* Write enable bit */
    uint32_t addr = BIT(31);

    /* Shift in the PCI device address and register offset. */
    addr = addr | (lbus << 16) | (ldev << 11) | (lfunc << 8) | (off & 0xFC);

    return addr;
}

uint32_t pci_read_32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off)
{
    uint32_t addr = pci_compute_port_address(bus, dev, func, off);

    /* Write the address into the "select" port, then the data will be available at the "data" port. */
    microkit_x86_ioport_write_32(pci_conf_addr_pio_id, pci_conf_addr_pio_addr, addr);
    return microkit_x86_ioport_read_32(pci_conf_data_pio_id, pci_conf_data_pio_addr);
}

void pci_write_8(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off, uint8_t data) {
    uint32_t addr = pci_compute_port_address(bus, dev, func, off);
    microkit_x86_ioport_write_32(pci_conf_addr_pio_id, pci_conf_addr_pio_addr, addr);
    microkit_x86_ioport_write_8(pci_conf_data_pio_id, pci_conf_data_pio_addr, data);
}

void pci_write_16(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off, uint16_t data) {
    uint32_t addr = pci_compute_port_address(bus, dev, func, off);
    microkit_x86_ioport_write_32(pci_conf_addr_pio_id, pci_conf_addr_pio_addr, addr);
    microkit_x86_ioport_write_16(pci_conf_data_pio_id, pci_conf_data_pio_addr, data);
}

void pci_write_32(uint8_t bus, uint8_t dev, uint8_t func, uint8_t off, uint32_t data)
{
    uint32_t addr = pci_compute_port_address(bus, dev, func, off);
    microkit_x86_ioport_write_32(pci_conf_addr_pio_id, pci_conf_addr_pio_addr, addr);
    microkit_x86_ioport_write_32(pci_conf_data_pio_id, pci_conf_data_pio_addr, data);
}

bool find_pci_device(uint8_t class, uint8_t subclass, uint8_t *bus, uint8_t *dev, uint8_t *func)
{
    int candidate_bus = 0;
    int candidate_dev = 0;
    int candidate_func = 0;

    for (; candidate_dev < 32; candidate_dev++) {
        for (; candidate_func < 8; candidate_func++) {
            uint32_t reg2 = pci_read_32(candidate_bus, candidate_dev, candidate_func, 0x8);
            uint8_t candidate_class = (reg2 >> 24) & 0xff;
            uint8_t candidate_subclass = (reg2 >> 16) & 0xff;

            if (reg2 != 0xffffffff && candidate_class == class && candidate_subclass == subclass) {
                *bus = candidate_bus;
                *dev = candidate_dev;
                *func = candidate_func;
                return true;
            }
        }
        candidate_func = 0;
    }

    return false;
}



/* Uncomment this to enable debug logging */
// #define DEBUG_PCI

#if defined(DEBUG_PCI)
#define LOG_PCI(...) do{ printf("%s|PCI: ", microkit_name); printf(__VA_ARGS__); }while(0)
#else
#define LOG_PCI(...) do{}while(0)
#endif

#define INTEL_82441_DEVICE_ID  0x1237

// TODO: use actual macros rather than hard-coded values
struct pci_bus pci_bus_state = {
    .host_bridge = { .device_id = INTEL_82441_DEVICE_ID, .vendor_id = 0x8086, .class_code = 0x6, .subclass = 0, .header_type = 0 },
    .isa_bridge = { .device_id = 0x7000,
                    .vendor_id = 0x8086,
                    .class_code = 0x6,
                    .subclass = 1,
                    .header_type = 0x80, // bit 7 multifunction
                },
    .ide_controller_enable = false,
};

static bool pci_host_bridge_addr_reg_enable(void)
{
    return !!(pci_bus_state.address_reg >> 31);
}

static uint8_t pci_host_bridge_addr_reg_bus(void)
{
    return (pci_bus_state.address_reg >> 16) & 0x7f;
}

static uint8_t pci_host_bridge_addr_reg_dev(void)
{
    return (pci_bus_state.address_reg >> 11) & 0x1f;
}

static uint8_t pci_host_bridge_addr_reg_func(void)
{
    return (pci_bus_state.address_reg >> 8) & 0x7;
}

static uint8_t pci_host_bridge_addr_reg_offset(void)
{
    return pci_bus_state.address_reg & 0xff;
}

static void pci_host_bridge_invalid_read(seL4_VCPUContext *vctx)
{
    uint64_t *vctx_raw = (uint64_t *)vctx;
    vctx_raw[RAX_IDX] = 0xffffffff;
}

bool is_pci_config_space_access_mech_1(uint16_t port_addr)
{
    if (port_addr >= PCI_CONFIG_ADDRESS_START_PORT && port_addr < PCI_CONFIG_ADDRESS_END_PORT) {
        return true;
    }
    if (port_addr >= PCI_CONFIG_DATA_START_PORT && port_addr < PCI_CONFIG_DATA_END_PORT) {
        return true;
    }
    return false;
}

static bool is_host_bridge_access(void)
{
    return pci_host_bridge_addr_reg_enable() && pci_host_bridge_addr_reg_bus() == 0
        && pci_host_bridge_addr_reg_dev() == 0 && pci_host_bridge_addr_reg_func() == 0;
}

static bool is_isa_bridge_access(void)
{
    return pci_host_bridge_addr_reg_enable() && pci_host_bridge_addr_reg_bus() == 0
        && pci_host_bridge_addr_reg_dev() == 1 && pci_host_bridge_addr_reg_func() == 0;
}

static bool is_isa_power_mgmt_access(void)
{
    return pci_host_bridge_addr_reg_enable() && pci_host_bridge_addr_reg_bus() == 0
        && pci_host_bridge_addr_reg_dev() == 1 && pci_host_bridge_addr_reg_func() == 3;
}

static bool is_ide_controller_access(void)
{
    return pci_host_bridge_addr_reg_enable() && pci_host_bridge_addr_reg_bus() == 0
        && pci_host_bridge_addr_reg_dev() == 1 && pci_host_bridge_addr_reg_func() == 1;
}

bool emulate_isa_power_mgmt_access(seL4_VCPUContext *vctx, bool is_read, ioport_access_width_t access_width) {
    // LOG_VMM("emulate_isa_power_mgmt_access is_read %d, offset 0x%x\n", is_read, pci_host_bridge_addr_reg_offset());

    uint64_t *vctx_raw = (uint64_t *)vctx;
    if (is_read) {
        switch (pci_host_bridge_addr_reg_offset()) {
            case PMREGMISC_OFFSET:
                vctx_raw[RAX_IDX] = pci_bus_state.isa_bridge_power_mgmt_regs.pmregmisc;
                break;
            case PMBA_OFFSET:
                vctx_raw[RAX_IDX] = pci_bus_state.isa_bridge_power_mgmt_regs.pmba;
                break;
        }
    } else {
        switch (pci_host_bridge_addr_reg_offset()) {
            case PMREGMISC_OFFSET:
                pci_bus_state.isa_bridge_power_mgmt_regs.pmregmisc = vctx_raw[RAX_IDX];
                break;
            case PMBA_OFFSET:
                pci_bus_state.isa_bridge_power_mgmt_regs.pmba = vctx_raw[RAX_IDX];
                LOG_VMM("pmba is 0x%lx\n", pci_bus_state.isa_bridge_power_mgmt_regs.pmba);
                break;
        }
    }

    return true;
}

bool emulate_pci_config_space_access_pio(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read,
                                         ioport_access_width_t access_width, struct pci_config_space *config_space)
{
    bool success = true;
    uint64_t *vctx_raw = (uint64_t *)vctx;
    uint8_t *config_bytes = (uint8_t *)config_space;
    int port_offset = port_addr - PCI_CONFIG_DATA_START_PORT;

    // caller must catch!
    assert(pci_host_bridge_addr_reg_enable());

    int bytes_to_copy = ioports_access_width_to_bytes(access_width);
    assert(bytes_to_copy);

    if (!is_read) {
        memcpy(&config_bytes[pci_host_bridge_addr_reg_offset() + port_offset], &vctx_raw[RAX_IDX], bytes_to_copy);
    } else {
        memcpy(&vctx_raw[RAX_IDX], &config_bytes[pci_host_bridge_addr_reg_offset() + port_offset], bytes_to_copy);
    }

    return success;
}

bool native_pci_config_space_access_pio(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read,
                                         ioport_access_width_t access_width)
{
    bool success = true;
    uint64_t *vctx_raw = (uint64_t *)vctx;
    int port_offset = port_addr - PCI_CONFIG_DATA_START_PORT;

    // caller must catch!
    assert(pci_host_bridge_addr_reg_enable());

    int bytes_to_copy = ioports_access_width_to_bytes(access_width);
    assert(bytes_to_copy);

    if (!is_read) {
        uint32_t addr = pci_compute_port_address(pci_bus_state.native_ide_con_addr.bus, pci_bus_state.native_ide_con_addr.dev,
                                                 pci_bus_state.native_ide_con_addr.func, pci_host_bridge_addr_reg_offset());
        microkit_x86_ioport_write_32(pci_conf_addr_pio_id, pci_conf_addr_pio_addr, addr);

        switch (access_width) {
        case IOPORT_BYTE_ACCESS_QUAL:
            microkit_x86_ioport_write_8(pci_conf_data_pio_id, pci_conf_data_pio_addr + port_offset, vctx_raw[RAX_IDX]);
            break;
        case IOPORT_WORD_ACCESS_QUAL:
            microkit_x86_ioport_write_16(pci_conf_data_pio_id, pci_conf_data_pio_addr + port_offset, vctx_raw[RAX_IDX]);
            break;
        case IOPORT_DWORD_ACCESS_QUAL:
            microkit_x86_ioport_write_32(pci_conf_data_pio_id, pci_conf_data_pio_addr + port_offset, vctx_raw[RAX_IDX]);
            break;
        }

    } else {
        uint32_t addr = pci_compute_port_address(pci_bus_state.native_ide_con_addr.bus, pci_bus_state.native_ide_con_addr.dev,
                                                 pci_bus_state.native_ide_con_addr.func, pci_host_bridge_addr_reg_offset());
        microkit_x86_ioport_write_32(pci_conf_addr_pio_id, pci_conf_addr_pio_addr, addr);

        switch (access_width) {
        case IOPORT_BYTE_ACCESS_QUAL:
            vctx_raw[RAX_IDX] = microkit_x86_ioport_read_8(pci_conf_data_pio_id, pci_conf_data_pio_addr + port_offset);
            break;
        case IOPORT_WORD_ACCESS_QUAL:
            vctx_raw[RAX_IDX] = microkit_x86_ioport_read_16(pci_conf_data_pio_id, pci_conf_data_pio_addr + port_offset);
            break;
        case IOPORT_DWORD_ACCESS_QUAL:
            vctx_raw[RAX_IDX] = microkit_x86_ioport_read_32(pci_conf_data_pio_id, pci_conf_data_pio_addr + port_offset);
            break;
        }

        // @billn hack ide
        if (pci_host_bridge_addr_reg_offset() == 0x4) {
            // disable bus master
            vctx_raw[RAX_IDX] &= ~BIT(2);
        }
        if (pci_host_bridge_addr_reg_offset() == 0x20) {
            // BAR4 = 0, no dma for ide
            vctx_raw[RAX_IDX] = 0;
        }
    }

    return success;
}


bool emulate_pci_config_space_access_mech_1(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read,
                                            ioport_access_width_t access_width)
{
    uint64_t *vctx_raw = (uint64_t *)vctx;

    // if (is_read) {
    //     LOG_PCI("handling PCI config space mech #1 read at I/O 0x%lx\n", port_addr);
    // } else {
    //     LOG_PCI("handling PCI config space mech #1 write at I/O 0x%lx, val 0x%lx\n", port_addr, vctx_raw[RAX_IDX]);
    // }

    bool success = true;

    if (port_addr >= PCI_CONFIG_ADDRESS_START_PORT && port_addr < PCI_CONFIG_ADDRESS_END_PORT) {
        if (is_read) {
            assert(port_addr == PCI_CONFIG_ADDRESS_START_PORT);
            vctx_raw[RAX_IDX] = pci_bus_state.address_reg;
        } else {
            uint32_t value = vctx_raw[RAX_IDX];
            if (value >> 31) {
                // @billn revisit
                // vcpu_print_regs(0);
                // assert(port_addr == PCI_CONFIG_ADDRESS_START_PORT);
                pci_bus_state.address_reg = value;

                LOG_PCI("selecting bus %d, device %d, func %d, reg_offset 0x%x\n", pci_host_bridge_addr_reg_bus(),
                        pci_host_bridge_addr_reg_dev(), pci_host_bridge_addr_reg_func(),
                        pci_host_bridge_addr_reg_offset());
            }
        }

    } else if (port_addr >= PCI_CONFIG_DATA_START_PORT && port_addr < PCI_CONFIG_DATA_END_PORT) {
        if (!pci_host_bridge_addr_reg_enable()) {
            pci_host_bridge_invalid_read(vctx);
        } else {

            // @billn probably not scalable in-terms of code readability
            if (is_host_bridge_access()) {
                success = emulate_pci_config_space_access_pio(vctx, port_addr, is_read, access_width,
                                                              &pci_bus_state.host_bridge);
            } else if (is_isa_bridge_access()) {
                success = emulate_pci_config_space_access_pio(vctx, port_addr, is_read, access_width,
                                                              &pci_bus_state.isa_bridge);

            } else if (is_isa_power_mgmt_access()) {
                success = emulate_isa_power_mgmt_access(vctx, is_read, access_width);

            } else if (pci_bus_state.ide_controller_enable && is_ide_controller_access()) {
                success = native_pci_config_space_access_pio(vctx, port_addr, is_read, access_width);
            } else {
                if (is_read) {
                    pci_host_bridge_invalid_read(vctx);
                }
            }
        }
    } else {
        success = false;
    }

    return success;
}

bool passthrough_ide_controller(uint64_t primary_ata_cmd_pio_id, uint64_t primary_ata_cmd_pio_addr,
                                uint64_t primary_ata_ctrl_pio_id, uint64_t primary_ata_ctrl_pio_addr,
                                uint64_t second_ata_cmd_pio_id, uint64_t second_ata_cmd_pio_addr,
                                uint64_t second_ata_ctrl_pio_id, uint64_t second_ata_ctrl_pio_addr)
{
    assert(primary_ata_cmd_pio_addr == 0x1f0);
    assert(primary_ata_ctrl_pio_addr == 0x3f6);
    assert(second_ata_cmd_pio_addr == 0x170);
    assert(second_ata_ctrl_pio_addr == 0x376);

    microkit_vcpu_x86_enable_ioport(GUEST_BOOT_VCPU_ID, primary_ata_cmd_pio_id, primary_ata_cmd_pio_addr, 8);
    microkit_vcpu_x86_enable_ioport(GUEST_BOOT_VCPU_ID, primary_ata_ctrl_pio_id, primary_ata_ctrl_pio_addr, 1);
    microkit_vcpu_x86_enable_ioport(GUEST_BOOT_VCPU_ID, second_ata_cmd_pio_id, second_ata_cmd_pio_addr, 8);
    microkit_vcpu_x86_enable_ioport(GUEST_BOOT_VCPU_ID, second_ata_ctrl_pio_id, second_ata_ctrl_pio_addr, 1);

    pci_bus_state.ide_controller_enable = true;

    // Find where the IDE controller lives on the host's PCI bus.
    assert(find_pci_device(1, 1, &pci_bus_state.native_ide_con_addr.bus, &pci_bus_state.native_ide_con_addr.dev,
                           &pci_bus_state.native_ide_con_addr.func));
    LOG_VMM("found host IDE controller @ PCI %d:%d.%d\n", pci_bus_state.native_ide_con_addr.bus,
            pci_bus_state.native_ide_con_addr.dev, pci_bus_state.native_ide_con_addr.func);

    return true;
}