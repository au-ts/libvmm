/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>
#include <libvmm/util/util.h>

/* Utility functions to decode a PIO VM Exit qualification. */
bool pio_fault_is_read(seL4_Word qualification);
bool pio_fault_is_write(seL4_Word qualification);
bool pio_fault_is_string_op(seL4_Word qualification);
uint16_t pio_fault_addr(seL4_Word qualification);
int pio_fault_to_access_width_bytes(seL4_Word qualification);

/* Emulate a string plus REP prefix operation on an IO Port, the caller provides the data bytes buffer
 * as `data` with length `data_len`. The function will copy up to `data_len` bytes into guest RAM according
 * to the architectural rules of the string input instruction.
 *
 * Returns the amount of bytes copied into guest RAM. The value is negative if guest set DF flag. */
int pio_emulate_string_read(uint64_t qualification, seL4_VCPUContext *vctx, uint8_t *data, size_t data_len);

/* Write the result of a port IO read emulation to vCPU general purpose registers
 * while respecting architectural rules. */
void pio_emulate_read(uint64_t qualification, seL4_VCPUContext *vctx, uint32_t data);

/* Retrieve the data of a port IO write instruction. */
uint32_t pio_get_write_data(uint64_t qualification, seL4_VCPUContext *vctx);

void emulate_ioport_noop_access(uint64_t qualification, seL4_VCPUContext *vctx);
