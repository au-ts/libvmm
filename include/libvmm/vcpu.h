/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void vcpu_set_suspend_pc(size_t vcpu_id, seL4_Word pc);
seL4_Word vcpu_get_suspend_pc(size_t vcpu_id);

void vcpu_fault_set_il(size_t vcpu_id, uint8_t il);
uint8_t vcpu_fault_get_il(size_t vcpu_id);

void vcpu_set_wfi(size_t vcpu_id, bool value);
bool vcpu_is_wfi(size_t vcpu_id);

void vcpu_reset(size_t vcpu_id);
void vcpu_print_regs(size_t vcpu_id);
