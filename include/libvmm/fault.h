/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <microkit.h>

#include <libvmm/util/util.h>

#if defined(CONFIG_ARCH_AARCH64)
#include <libvmm/arch/aarch64/fault.h>
#elif defined(CONFIG_ARCH_RISCV)
#include <libvmm/arch/riscv/fault.h>
#endif

typedef bool (*vm_exception_handler_t)(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data);
bool fault_register_vm_exception_handler(uintptr_t base, size_t size, vm_exception_handler_t callback, void *data);

bool fault_handle_vm_exception(size_t vcpu_id);
