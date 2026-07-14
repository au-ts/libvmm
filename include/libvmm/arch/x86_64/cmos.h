/*
 * Copyright 2026, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string.h>
#include <stddef.h>
#include <stdbool.h>

bool initialise_cmos(void);

bool cmos_set_ram_byte(uint8_t offset, uint8_t value);