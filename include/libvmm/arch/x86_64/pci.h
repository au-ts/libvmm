/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>
#include <libvmm/arch/x86_64/ioports.h>

#define PCI_CONFIG_ADDRESS_START_PORT 0xCF8
#define PCI_CONFIG_ADDRESS_END_PORT (PCI_CONFIG_ADDRESS_START_PORT + 4)

#define PCI_CONFIG_DATA_START_PORT 0xCFC
#define PCI_CONFIG_DATA_END_PORT (PCI_CONFIG_DATA_START_PORT + 4)

bool is_pci_config_space_access_mech_1(uint16_t port_addr);
bool emulate_pci_config_space_access_mech_1(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read,
                                            ioport_access_width_t access_width);

bool passthrough_ide_controller(uint64_t primary_ata_cmd_pio_id, uint64_t primary_ata_cmd_pio_addr,
                                uint64_t primary_ata_ctrl_pio_id, uint64_t primary_ata_ctrl_pio_addr,
                                uint64_t second_ata_cmd_pio_id, uint64_t second_ata_cmd_pio_addr,
                                uint64_t second_ata_ctrl_pio_id, uint64_t second_ata_ctrl_pio_addr);
