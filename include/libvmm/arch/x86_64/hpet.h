/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <sel4/sel4.h>
#include <libvmm/arch/x86_64/instruction.h>

bool hpet_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification,
                       decoded_instruction_ret_t decoded_ins);