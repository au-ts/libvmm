/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <stdbool.h>

#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/vmcs.h>

bool vcpu_init(size_t vcpu_id, uintptr_t rip, uintptr_t rsp) {
    LOG_VMM("vcpu_init: vcpu_id %d, rip 0x%x, rsp 0x%x\n", vcpu_id, rip, rsp);
}