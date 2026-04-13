/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>
#include <libvmm/util/util.h>

bool pio_fault_is_read(seL4_Word qualification);
bool pio_fault_is_write(seL4_Word qualification);
bool pio_fault_is_string_op(seL4_Word qualification);
uint16_t pio_fault_addr(seL4_Word qualification);
int pio_fault_to_access_width_bytes(seL4_Word qualification);

bool emulate_ioports(seL4_VCPUContext *vctx, uint64_t f_qualification);
