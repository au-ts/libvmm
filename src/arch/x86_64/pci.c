/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>
#include <sddf/util/custom_libc/string.h>
#include <libvmm/arch/x86_64/pci.h>
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <libvmm/virtio/pci.h>

/* Uncomment this to enable debug logging */
// #define DEBUG_PCI

#if defined(DEBUG_PCI)
#define LOG_PCI(...) do{ printf("%s|PCI: ", microkit_name); printf(__VA_ARGS__); }while(0)
#else
#define LOG_PCI(...) do{}while(0)
#endif

struct pci_host_bridge {
    uint32_t address_reg;

    struct pci_config_space self_config_space;
};

static struct pci_host_bridge pci_host_bridge = { .self_config_space = { .device_id = 0x1234,
                                                                         .vendor_id = 0x5e14,
                                                                         .class_code = 0x6,
                                                                         .subclass = 0,
                                                                         .header_type = 0 } };

static bool pci_host_bridge_addr_reg_enable(void)
{
    return !!(pci_host_bridge.address_reg >> 31);
}

static uint8_t pci_host_bridge_addr_reg_bus(void)
{
    return (pci_host_bridge.address_reg >> 16) & 0x7f;
}

static uint8_t pci_host_bridge_addr_reg_dev(void)
{
    return (pci_host_bridge.address_reg >> 11) & 0x1f;
}

static uint8_t pci_host_bridge_addr_reg_func(void)
{
    return (pci_host_bridge.address_reg >> 8) & 0x7;
}

static uint8_t pci_host_bridge_addr_reg_offset(void)
{
    return pci_host_bridge.address_reg & 0xff;
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
    return pci_host_bridge_addr_reg_enable() && pci_host_bridge_addr_reg_bus() == 0 && pci_host_bridge_addr_reg_dev() == 0
        && pci_host_bridge_addr_reg_func() == 0;
}

bool emulate_pci_host_bridge_access(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read,
                                    ioport_access_width_t access_width)
{
    bool success = true;
    if (!is_read) {
        return success;
    }

    uint64_t *vctx_raw = (uint64_t *)vctx;
    int port_offset = port_addr - PCI_CONFIG_DATA_START_PORT;

    assert(pci_host_bridge_addr_reg_enable());

    uint8_t *config_space_bytes = (uint8_t *)(&pci_host_bridge.self_config_space);
    int bytes_to_copy = ioports_access_width_to_bytes(access_width);
    assert(bytes_to_copy);

    memcpy(&vctx_raw[RAX_IDX], &config_space_bytes[pci_host_bridge_addr_reg_offset() + port_offset], bytes_to_copy);

    LOG_PCI("read host bridge, offset 0x%x, val 0x%x, read width: %d bytes\n", pci_host_bridge_addr_reg_offset() + port_offset, vctx_raw[RAX_IDX], bytes_to_copy);

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
            vctx_raw[RAX_IDX] = pci_host_bridge.address_reg;
        } else {
            uint32_t value = vctx_raw[RAX_IDX];
            if (value >> 31) {
                assert(port_addr == PCI_CONFIG_ADDRESS_START_PORT);
                pci_host_bridge.address_reg = value;

                LOG_PCI("selecting bus %d, device %d, func %d, reg_offset 0x%x\n", pci_host_bridge_addr_reg_bus(),
                        pci_host_bridge_addr_reg_dev(), pci_host_bridge_addr_reg_func(),
                        pci_host_bridge_addr_reg_offset());
            }
        }

    } else if (port_addr >= PCI_CONFIG_DATA_START_PORT && port_addr < PCI_CONFIG_DATA_END_PORT) {
        if (!pci_host_bridge_addr_reg_enable()) {
            pci_host_bridge_invalid_read(vctx);
        } else {
            if (is_host_bridge_access()) {
                success = emulate_pci_host_bridge_access(vctx, port_addr, is_read, access_width);
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