/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

bool register_qemu_bochs_display_on_pci_bus(uint8_t bus, uint8_t dev, uint8_t func);
