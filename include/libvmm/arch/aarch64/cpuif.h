/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <libvmm/util/util.h>

typedef bool (*sysreg_access_fn_t)(size_t vcpu_id, seL4_UserContext *regs, bool is_read);
typedef bool (*sysreg_read_exception_handler_t)(size_t vcpu_id, seL4_UserContext *regs);
typedef bool (*sysreg_write_exception_handler_t)(size_t vcpu_id, seL4_UserContext *regs, uint64_t data);

/* Handles exception from MSR, MRS, or System instruction execution in AArch64 state (EC class 0x18). */
bool handle_sysreg_64_fault(size_t vcpu_id, uint64_t hsr, seL4_UserContext *regs);