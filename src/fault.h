/*
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <stdint.h>
#include <sel4cp.h>

bool fault_advance_vcpu(seL4_UserContext *regs);
bool fault_advance(seL4_UserContext *regs, uint64_t addr, uint64_t fsr, uint64_t reg_val);
uint64_t fault_get_data_mask(uint64_t addr, uint64_t fsr);
uint64_t fault_get_data(seL4_UserContext *regs, uint64_t fsr);
uint64_t fault_emulate(seL4_UserContext *regs, uint64_t reg, uint64_t addr, uint64_t fsr, uint64_t reg_val);

bool fault_is_write(uint64_t fsr);
bool fault_is_read(uint64_t fsr);
