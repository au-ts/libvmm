/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>

#define PCI_CONFIG_ADDRESS_START_PORT 0xCF8
#define PCI_CONFIG_ADDRESS_PORT_SIZE 4
#define PCI_CONFIG_ADDRESS_END_PORT (PCI_CONFIG_ADDRESS_START_PORT + PCI_CONFIG_ADDRESS_PORT_SIZE)

#define PCI_CONFIG_DATA_START_PORT 0xCFC
#define PCI_CONFIG_DATA_PORT_SIZE 4
#define PCI_CONFIG_DATA_END_PORT (PCI_CONFIG_DATA_START_PORT + PCI_CONFIG_DATA_PORT_SIZE)

bool pci_x86_init(void);
