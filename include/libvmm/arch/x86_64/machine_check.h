/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

/* Virtualisation of the x86 MCA and MCE */

bool msr_is_machine_check(uint64_t msr, bool is_read);

bool handle_machine_check_msr_read(uint64_t msr, uint64_t *result);

bool handle_machine_check_msr_write(uint64_t msr, uint64_t value);