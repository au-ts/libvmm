/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <sel4/sel4.h>

typedef bool (*vm_exception_handler_t)(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data);
bool fault_register_vm_exception_handler(uintptr_t base, size_t size, vm_exception_handler_t callback, void *data);

bool fault_handle_registered_vm_exceptions(size_t vcpu_id, uintptr_t addr, size_t fsr, seL4_UserContext *regs);

bool fault_is_write(seL4_Word fsr);
bool fault_is_read(seL4_Word fsr);
