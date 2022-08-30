/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <sel4cp.h>

// SMC VCPU fault handler
bool handle_smc();

// Helper functions
void smc_set_return_value(seL4_UserContext *u, uint64_t val);
uint64_t smc_get_arg(seL4_UserContext *u, uint64_t arg);
