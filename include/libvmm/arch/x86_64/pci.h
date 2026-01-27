/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>

#define PCI_CONFIG_ADDRESS_START_PORT 0xCF8
#define PCI_CONFIG_ADDRESS_PORT_SIZE 4
#define PCI_CONFIG_ADDRESS_END_PORT (PCI_CONFIG_ADDRESS_START_PORT + PCI_CONFIG_ADDRESS_PORT_SIZE)

#define PCI_CONFIG_DATA_START_PORT 0xCFC
#define PCI_CONFIG_DATA_PORT_SIZE 4
#define PCI_CONFIG_DATA_END_PORT (PCI_CONFIG_DATA_START_PORT + PCI_CONFIG_DATA_PORT_SIZE)

struct pci_x86_passthrough {
    bool valid;
    uint64_t pci_conf_addr_pio_id;
    uint64_t pci_conf_data_pio_id;

    bool ata_passthrough;
    // Real
    uint16_t ata_bus;
    uint16_t ata_dev;
    uint16_t ata_func;
    // Guest
    uint16_t ata_v_bus;
    uint16_t ata_v_dev;
    uint16_t ata_v_func;
};

bool pci_x86_init(void);

bool pci_x86_enable_passthrough_devices(uint64_t pci_conf_addr_pio_id, uint64_t pci_conf_addr_pio_addr,
                                        uint64_t pci_conf_addr_pio_size, uint64_t pci_conf_data_pio_id,
                                        uint64_t pci_conf_data_pio_addr, uint64_t pci_conf_data_pio_size);

bool pci_x86_passthrough_ata_controller(uint64_t primary_ata_cmd_pio_id, uint64_t primary_ata_cmd_pio_addr,
                                        uint64_t primary_ata_ctrl_pio_id, uint64_t primary_ata_ctrl_pio_addr,
                                        uint64_t second_ata_cmd_pio_id, uint64_t second_ata_cmd_pio_addr,
                                        uint64_t second_ata_ctrl_pio_id, uint64_t second_ata_ctrl_pio_addr);

bool pci_x86_config_space_read_from_native(int access_width_bytes, uint8_t bus, uint8_t dev, uint8_t func, uint16_t reg_offset,
                                           uint32_t *data);

bool pci_x86_config_space_write_to_native(int access_width_bytes, uint8_t bus, uint8_t dev, uint8_t func, uint16_t reg_offset,
                                          uint32_t data);