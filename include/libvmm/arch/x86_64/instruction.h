/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stddef.h>
#include <microkit.h>

// Index of registers in a `seL4_VCPUContext`.
typedef enum register_idx {
    RAX_IDX = 0,
    RBX_IDX,
    RCX_IDX,
    RDX_IDX,
    RSI_IDX,
    RDI_IDX,
    RBP_IDX,
    R8_IDX,
    R9_IDX,
    R10_IDX,
    R11_IDX,
    R12_IDX,
    R13_IDX,
    R14_IDX,
    R15_IDX
} register_idx_t;

typedef struct decode_fail_instruction_data {
} decode_fail_instruction_data_t;

typedef enum memory_access_width {
    BYTE_ACCESS_WIDTH,  // 1-byte
    WORD_ACCESS_WIDTH,  // 2-byte
    DWORD_ACCESS_WIDTH, // 4-byte
    QWORD_ACCESS_WIDTH, // 8-byte
} memory_access_width_t;

typedef struct memory_instruction_data {
    // We don't store the memory access direction as the CPU would
    // tell us via the fault qualification.
    bool zero_extend;
    register_idx_t target_reg;
    memory_access_width_t access_width;
} memory_instruction_data_t;

typedef struct write_imm_instruction_data {
    uint64_t value;
    memory_access_width_t access_width;
} write_imm_instruction_data_t;

typedef union instruction_data {
    decode_fail_instruction_data_t decode_fail;
    memory_instruction_data_t memory_instruction;
    write_imm_instruction_data_t write_imm_instruction;
} instruction_data_t;

typedef enum instruction_type {
    INSTRUCTION_DECODE_FAIL,
    INSTRUCTION_MEMORY,
    INSTRUCTION_WRITE_IMM,
} instruction_type_t;

#define X86_MAX_INSTRUCTION_LENGTH 15

typedef struct decoded_instruction_ret {
    /* For debugging purpose */
    char raw[X86_MAX_INSTRUCTION_LENGTH];
    int raw_len;

    instruction_type_t type;
    instruction_data_t decoded;
} decoded_instruction_ret_t;

decoded_instruction_ret_t decode_instruction(size_t vcpu_id, seL4_Word rip, seL4_Word instruction_len);

void debug_print_instruction(decoded_instruction_ret_t decode_result);

int mem_access_width_to_bytes(decoded_instruction_ret_t decoded_ins);

bool mem_write_get_data(decoded_instruction_ret_t decoded_ins, size_t ept_fault_qualification, seL4_VCPUContext *vctx,
                        uint64_t *ret);

bool mem_read_set_data(decoded_instruction_ret_t decoded_ins, size_t ept_fault_qualification, seL4_VCPUContext *vctx,
                       uint64_t data);