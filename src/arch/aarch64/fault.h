/*
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sel4cp.h>

/* Fault-handling functions */
bool fault_handle_vcpu_exception(size_t vcpu_id);
bool fault_handle_vppi_event(size_t vcpu_id);
bool fault_handle_user_exception(size_t vcpu_id);
bool fault_handle_unknown_syscall(size_t vcpu_id);
bool fault_handle_vm_exception(size_t vcpu_id);

/* Helpers for emulating the fault and getting fault details */
bool fault_advance_vcpu(size_t vcpu_id, seL4_UserContext *regs);
bool fault_advance(size_t vcpu_id, seL4_UserContext *regs, uint64_t addr, uint64_t fsr, uint64_t reg_val);
uint64_t fault_get_data_mask(uint64_t addr, uint64_t fsr);
uint64_t fault_get_data(seL4_UserContext *regs, uint64_t fsr);
uint64_t fault_emulate(seL4_UserContext *regs, uint64_t reg, uint64_t addr, uint64_t fsr, uint64_t reg_val);

/* Take the fault label given by the kernel and convert it to a string. */
char *fault_to_string(seL4_Word fault_label);

bool fault_is_write(uint64_t fsr);
bool fault_is_read(uint64_t fsr);
