/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>
// TODO: need pci_config_space from virtio pci.h, should probably extract into common header
#include <libvmm/virtio/pci.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <libvmm/arch/x86_64/ioports.h>

#define PCI_CONFIG_ADDRESS_START_PORT 0xCF8
#define PCI_CONFIG_ADDRESS_END_PORT (PCI_CONFIG_ADDRESS_START_PORT + 4)

#define PCI_CONFIG_DATA_START_PORT 0xCFC
#define PCI_CONFIG_DATA_END_PORT (PCI_CONFIG_DATA_START_PORT + 4)

struct pci_device_address {
    uint8_t dev;
    uint8_t bus;
    uint8_t func;
};

// function 3 of isa bridge
// https://www.intel.com/Assets/PDF/datasheet/290562.pdf
#define PMBA_OFFSET 0x40
#define PMREGMISC_OFFSET 0x80

struct isa_bridge_power_mgmt_regs {
    uint32_t pmba;
    uint8_t pmregmisc;
};

struct pci_bus {
    uint32_t selected_pio_addr_reg;

    

    struct pci_config_space host_bridge;
    struct pci_config_space isa_bridge;
    struct isa_bridge_power_mgmt_regs isa_bridge_power_mgmt_regs;

    bool ide_controller_enable;
    struct pci_device_address native_ide_con_addr;
};

bool is_pci_config_space_access_mech_1(uint16_t port_addr);
bool emulate_pci_config_space_access_mech_1(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read,
                                            ioport_access_width_t access_width);

bool emulate_pci_config_space_access_ecam(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification,
                                          memory_instruction_data_t decoded_mem_ins);

bool passthrough_ide_controller(uint64_t primary_ata_cmd_pio_id, uint64_t primary_ata_cmd_pio_addr,
                                uint64_t primary_ata_ctrl_pio_id, uint64_t primary_ata_ctrl_pio_addr,
                                uint64_t second_ata_cmd_pio_id, uint64_t second_ata_cmd_pio_addr,
                                uint64_t second_ata_ctrl_pio_id, uint64_t second_ata_ctrl_pio_addr);
