/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Virtualisation of the x86 Memory Type Range Registers */

bool initialise_mtrr(void);
bool msr_is_mtrr(uint64_t msr, bool is_read);
bool handle_mtrr_msr_read(uint64_t msr, uint64_t *result);
bool handle_mtrr_msr_write(uint64_t msr, uint64_t value);
