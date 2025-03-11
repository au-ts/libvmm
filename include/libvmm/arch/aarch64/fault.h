/*
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <microkit.h>

/* Fault-handling functions */
bool fault_handle(size_t vcpu_id, microkit_msginfo msginfo);

bool fault_handle_vcpu_exception(size_t vcpu_id);
bool fault_handle_vppi_event(size_t vcpu_id);
bool fault_handle_user_exception(size_t vcpu_id);
bool fault_handle_unknown_syscall(size_t vcpu_id);
bool fault_handle_vm_exception(size_t vcpu_id);

typedef bool (*vm_exception_handler_t)(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data);
bool fault_register_vm_exception_handler(uintptr_t base, size_t size, vm_exception_handler_t callback, void *data);

/* Helpers for emulating the fault and getting fault details */
bool fault_advance_vcpu(size_t vcpu_id, seL4_UserContext *regs, size_t regs_size);
bool fault_advance(size_t vcpu_id, seL4_UserContext *regs, uint64_t addr, uint64_t fsr, uint64_t reg_val);
uint64_t fault_get_data_mask(uint64_t addr, uint64_t fsr);
uint64_t fault_get_data(seL4_UserContext *regs, uint64_t fsr);
uint64_t fault_emulate(seL4_UserContext *regs, uint64_t reg, uint64_t addr, uint64_t fsr, uint64_t reg_val);
void fault_emulate_write(seL4_UserContext *regs, size_t addr, size_t fsr, size_t reg_val);

/* Take the fault label given by the kernel and convert it to a string. */
char *fault_to_string(seL4_Word fault_label);

bool fault_is_write(uint64_t fsr);
bool fault_is_read(uint64_t fsr);
