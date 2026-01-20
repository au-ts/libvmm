/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <string.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <libvmm/arch/x86_64/util.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>

// https://wiki.osdev.org/X86-64_Instruction_Encoding

#define X86_MAX_INSTRUCTION_LENGTH 15

// https://c9x.me/x86/html/file_module_x86_id_176.html
#define NUM_MOV_OPCODES 4
#define OPCODE_MOV_BYTE_TO_MEM 0x88
#define OPCODE_MOV_WORD_TO_MEM 0x89
#define OPCODE_MOV_BYTE_FROM_MEM 0x8a
#define OPCODE_MOV_WORD_FROM_MEM 0x8b
static uint8_t mov_opcodes[NUM_MOV_OPCODES] = { OPCODE_MOV_BYTE_TO_MEM, OPCODE_MOV_WORD_TO_MEM,
                                                OPCODE_MOV_BYTE_FROM_MEM, OPCODE_MOV_WORD_FROM_MEM };

static uint8_t opcode_movzx_byte_from_mem[2] = { 0x0f, 0xb6 };
static uint8_t opcode_movzx_word_from_mem[2] = { 0x0f, 0xb7 };

int mem_access_width_to_bytes(memory_access_width_t access_width)
{
    switch (access_width) {
    case BYTE_ACCESS_WIDTH:
        return 1;
    case WORD_ACCESS_WIDTH:
        return 2;
    case DWORD_ACCESS_WIDTH:
        return 4;
    case QWORD_ACCESS_WIDTH:
        return 8;
    default:
        return 0;
    }
}

static register_idx_t modrm_reg_to_vctx_idx(uint8_t reg, bool rex_r)
{
    // the REX.R prefix will expand the register index to be 4-bits wide
    uint8_t idx = (reg & 0x7) | (rex_r ? 0x8 : 0);

    switch (idx) {
    case 0:
        return RAX_IDX;
    case 1:
        return RCX_IDX;
    case 2:
        return RDX_IDX;
    case 3:
        return RBX_IDX;
    case 5:
        return RBP_IDX;
    case 6:
        return RSI_IDX;
    case 7:
        return RDI_IDX;
    case 8:
        return R8_IDX;
    case 9:
        return R9_IDX;
    case 10:
        return R10_IDX;
    case 11:
        return R11_IDX;
    case 12:
        return R12_IDX;
    case 13:
        return R13_IDX;
    case 14:
        return R14_IDX;
    case 15:
        return R15_IDX;
    default:
        LOG_VMM_ERR("unknown register idx: 0x%x, reg: 0x%x, REX.R: %d\n", idx, reg, rex_r);
        assert(false);
    }

    return -1;
}

decoded_instruction_ret_t decode_instruction(size_t vcpu_id, seL4_Word rip, seL4_Word instruction_len)
{
    uint8_t instruction_buf[X86_MAX_INSTRUCTION_LENGTH];
    memset(instruction_buf, 0, X86_MAX_INSTRUCTION_LENGTH);

    uint64_t rip_gpa;
    int bytes_remaining;
    assert(gva_to_gpa(vcpu_id, rip, &rip_gpa, &bytes_remaining));

    // @billn fix lazyness
    assert(bytes_remaining >= instruction_len);

    assert(instruction_len <= X86_MAX_INSTRUCTION_LENGTH);
    memcpy(instruction_buf, gpa_to_vaddr(rip_gpa), instruction_len);

    // @billn I really think something more "industrial grade" should be used for a job like this.
    // Such as https://github.com/zyantific/zydis which is no-malloc and no-libc, but it uses cmake...yuck
    // But then we introduce a dependency...
    decoded_instruction_ret_t ret = { .type = INSTRUCTION_DECODE_FAIL, .decoded = {} };

    bool opcode_valid = false;
    int parsed_byte = 0;
    bool rex_w = false; // 64-bit operand size
    bool rex_r = false; // 4-bit operand, rather than 3-bit
    bool rex_x = false; // 4-bit SIB.index
    bool rex_b = false; // 4-bit MODRM.rm field or the SIB.base field
    bool opd_size_override = false;

    for (int i = 0; i < instruction_len; i++) {
        // scan for REX byte, which is always 0b0100WRXB
        if (instruction_buf[parsed_byte] >> 4 == 4) {
            uint8_t rex_byte = instruction_buf[parsed_byte];
            parsed_byte += 1;
    
            rex_w = (rex_byte & BIT(3)) != 0;
            rex_r = (rex_byte & BIT(2)) != 0;
            rex_x = (rex_byte & BIT(1)) != 0;
            rex_b = (rex_byte & BIT(0)) != 0;
        }
        
        // scan for the "operand-size override" prefix, which switch the operand size
        // from 32 to 16 bits, though the REX prefix have more precendence
        if (instruction_buf[parsed_byte] == 0x66) {
            opd_size_override = true;
            parsed_byte += 1;
        }
    }

    // match the opcode against a list of known opcodes that we provide decoding logic for.
    // An opcode can be multi-bytes, first match for 1 byte opcode
    for (int i = 0; i < NUM_MOV_OPCODES; i++) {
        if (instruction_buf[parsed_byte] == mov_opcodes[i]) {
            opcode_valid = true;
            uint8_t opcode = instruction_buf[parsed_byte];

            // An opcode byte is followed by a ModR/M byte to encode src/dest register.
            uint8_t modrm = instruction_buf[parsed_byte + 1];
            // uint8_t mod = modrm >> 6;
            uint8_t reg = (modrm >> 3) & 0x7;
            // uint8_t rm = modrm & 0x7;

            ret.type = INSTRUCTION_MEMORY;
            if (rex_w) {
                ret.decoded.memory_instruction.access_width = QWORD_ACCESS_WIDTH;
            } else {
                switch (opcode) {
                case OPCODE_MOV_BYTE_TO_MEM:
                case OPCODE_MOV_BYTE_FROM_MEM:
                    ret.decoded.memory_instruction.access_width = BYTE_ACCESS_WIDTH;
                    break;
                case OPCODE_MOV_WORD_TO_MEM:
                case OPCODE_MOV_WORD_FROM_MEM:
                    if (opd_size_override) {
                        ret.decoded.memory_instruction.access_width = WORD_ACCESS_WIDTH;
                    } else {
                        ret.decoded.memory_instruction.access_width = DWORD_ACCESS_WIDTH;
                    }
                    break;
                default:
                    LOG_VMM_ERR("internal bug: unhandled 1 byte mov opcode 0x%x\n", opcode);
                    assert(false);
                    break;
                }
            }
            ret.decoded.memory_instruction.zero_extend = false;
            ret.decoded.memory_instruction.target_reg = modrm_reg_to_vctx_idx(reg, rex_r);

            break;
        }
    }

    // no match, now try the 2 bytes movzx opcodes
    if (instruction_len - parsed_byte >= 3) {
        if (instruction_buf[parsed_byte] == opcode_movzx_byte_from_mem[0]
            && instruction_buf[parsed_byte + 1] == opcode_movzx_byte_from_mem[1]) {
            opcode_valid = true;
            uint8_t modrm = instruction_buf[parsed_byte + 2];
            uint8_t reg = (modrm >> 3) & 0x7;
            ret.type = INSTRUCTION_MEMORY;
            ret.decoded.memory_instruction.access_width = BYTE_ACCESS_WIDTH;
            ret.decoded.memory_instruction.zero_extend = true;
            ret.decoded.memory_instruction.target_reg = modrm_reg_to_vctx_idx(reg, rex_r);
        } else if (instruction_buf[parsed_byte] == opcode_movzx_word_from_mem[0]
                   && instruction_buf[parsed_byte + 1] == opcode_movzx_word_from_mem[1]) {
            opcode_valid = true;
            uint8_t modrm = instruction_buf[parsed_byte + 2];
            uint8_t reg = (modrm >> 3) & 0x7;
            ret.type = INSTRUCTION_MEMORY;
            ret.decoded.memory_instruction.access_width = WORD_ACCESS_WIDTH;
            ret.decoded.memory_instruction.zero_extend = true;
            ret.decoded.memory_instruction.target_reg = modrm_reg_to_vctx_idx(reg, rex_r);
        }
    }

    if (!opcode_valid) {
        LOG_VMM_ERR("uh-oh can't decode instruction, bail\n");
        for (int i = 0; i < instruction_len; i++) {
            LOG_VMM_ERR("ins byte 0x%x\n", instruction_buf[i]);
        }
        vcpu_print_regs(0);
        assert(opcode_valid);
    }

    // LOG_VMM("raw instruction:\n");
    // for (int i = 0; i < instruction_len; i++) {
    //     LOG_VMM("[%d]: 0x%x\n", i, instruction_buf[i]);
    // }
    return ret;
}