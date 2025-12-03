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

#define TIMER_DRV_CH_FOR_HPET 12

void hpet_handle_timer_ntfn(void);

bool hpet_fault_handle(seL4_VCPUContext *vctx, uint64_t offset, seL4_Word qualification,
                        memory_instruction_data_t decoded_mem_ins);