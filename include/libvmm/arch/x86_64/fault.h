/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <microkit.h>

#include <libvmm/arch/x86_64/instruction.h>

/* Documents referenced:
 * 1. Intel® 64 and IA-32 Architectures Software Developer’s Manual
 *    Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *    Order Number: 325462-080US June 2023
 */

/* [1] "Table 6-1. Exceptions and Interrupts" */
#define GP_VECTOR 13

char *fault_to_string(int exit_reason);

bool fault_handle(size_t vcpu_id);

typedef bool (*ept_exception_callback_t)(size_t vcpu_id, size_t offset, size_t qualification,
                                         decoded_instruction_ret_t decoded_ins, seL4_VCPUContext *vctx, void *cookie);

bool fault_update_ept_exception_handler(uintptr_t base, uintptr_t new_base);
bool fault_register_ept_exception_handler(uintptr_t base, size_t size, ept_exception_callback_t callback, void *cookie);

typedef bool (*pio_exception_callback_t)(size_t vcpu_id, uint16_t port_offset, size_t qualification,
                                         seL4_VCPUContext *vctx, void *cookie);
bool fault_register_pio_exception_handler(uint16_t base, uint16_t size, pio_exception_callback_t callback,
                                          void *cookie);