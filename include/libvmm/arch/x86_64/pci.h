/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <stdint.h>

/* Document referenced: https://wiki.osdev.org/PCI */

#define PCI_CONFIG_ADDRESS_START_PORT 0xCF8
#define PCI_CONFIG_ADDRESS_PORT_SIZE 4
#define PCI_CONFIG_ADDRESS_END_PORT (PCI_CONFIG_ADDRESS_START_PORT + PCI_CONFIG_ADDRESS_PORT_SIZE)

#define PCI_CONFIG_DATA_START_PORT 0xCFC
#define PCI_CONFIG_DATA_PORT_SIZE 4
#define PCI_CONFIG_DATA_END_PORT (PCI_CONFIG_DATA_START_PORT + PCI_CONFIG_DATA_PORT_SIZE)

bool pci_pio_addr_reg_enable(uint32_t select_reg)
{
    return !!(select_reg >> 31);
}

uint8_t pci_pio_addr_reg_bus(uint32_t select_reg)
{
    return (select_reg >> 16) & 0x7f;
}

uint8_t pci_pio_addr_reg_dev(uint32_t select_reg)
{
    return (select_reg >> 11) & 0x1f;
}

uint8_t pci_pio_addr_reg_func(uint32_t select_reg)
{
    return (select_reg >> 8) & 0x7;
}

uint8_t pci_pio_addr_reg_offset(uint32_t select_reg)
{
    return select_reg & 0xff;
}
