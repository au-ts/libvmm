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
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/util/util.h>
#include <libvmm/guest.h>
#include <libvmm/guest_ram.h>
#include <sddf/util/util.h>

/* Documents referenced:
 * https://wiki.osdev.org/X86-64_Instruction_Encoding
 * https://c9x.me/x86/html/file_module_x86_id_176.html
 */

#define NUM_MOV_OPCODES 4
#define OPCODE_MOV_BYTE_TO_MEM 0x88
#define OPCODE_MOV_WORD_TO_MEM 0x89
#define OPCODE_MOV_BYTE_FROM_MEM 0x8a
#define OPCODE_MOV_WORD_FROM_MEM 0x8b
static uint8_t mov_opcodes[NUM_MOV_OPCODES] = { OPCODE_MOV_BYTE_TO_MEM, OPCODE_MOV_WORD_TO_MEM,
                                                OPCODE_MOV_BYTE_FROM_MEM, OPCODE_MOV_WORD_FROM_MEM };

#define NUM_MOV_IMM_OPCODES 1
#define OPCODE_MOV_IMM_WORD_TO_MEM 0xc7
static uint8_t mov_imm_opcodes[NUM_MOV_IMM_OPCODES] = { OPCODE_MOV_IMM_WORD_TO_MEM };

static uint8_t opcode_movzx_byte_from_mem[2] = { 0x0f, 0xb6 };
static uint8_t opcode_movzx_word_from_mem[2] = { 0x0f, 0xb7 };

static int access_width_decode(memory_access_width_t access_width)
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
        LOG_VMM_ERR("access_width_decode() unknown access width %d\n", access_width);
        return 0;
    }
}

int mem_access_width_to_bytes(decoded_instruction_ret_t decoded_ins)
{
    int access_width_bytes;
    switch (decoded_ins.type) {
    case INSTRUCTION_MEMORY:
        access_width_bytes = access_width_decode(decoded_ins.decoded.memory_instruction.access_width);
        break;
    case INSTRUCTION_WRITE_IMM:
        access_width_bytes = access_width_decode(decoded_ins.decoded.write_imm_instruction.access_width);
        break;
    default:
        access_width_bytes = 0;
        LOG_VMM_ERR("mem_access_width_to_bytes() not a memory access instruction\n");
        break;
    }
    return access_width_bytes;
}

bool mem_write_get_data(decoded_instruction_ret_t decoded_ins, size_t ept_fault_qualification, seL4_VCPUContext *vctx,
                        uint64_t *ret)
{
    uint64_t *vctx_raw = (uint64_t *)vctx;
    bool success = true;

    int access_width_bytes = mem_access_width_to_bytes(decoded_ins);
    assert(access_width_bytes != 0);
    assert(access_width_bytes <= 8);

    switch (decoded_ins.type) {
    case INSTRUCTION_MEMORY:
        if (ept_fault_is_read(ept_fault_qualification)) {
            LOG_VMM_ERR("mem_write_get_data() got a read fault\n");
            success = false;
        } else {
            *ret = vctx_raw[decoded_ins.decoded.memory_instruction.target_reg];
        }
        break;
    case INSTRUCTION_WRITE_IMM:
        assert(ept_fault_is_write(ept_fault_qualification));
        *ret = decoded_ins.decoded.write_imm_instruction.value;
        break;
    default:
        LOG_VMM_ERR("mem_write_get_data() not a memory access instruction\n");
        return false;
    }

    switch (access_width_bytes) {
    case 1:
        *ret &= 0xff;
        break;
    case 2:
        *ret &= 0xffff;
        break;
    case 4:
        *ret &= 0xffffffff;
        break;
    }

    return success;
}

bool mem_read_set_data(decoded_instruction_ret_t decoded_ins, size_t ept_fault_qualification, seL4_VCPUContext *vctx,
                       uint64_t data)
{
    if (ept_fault_is_write(ept_fault_qualification)) {
        LOG_VMM_ERR("mem_read_set_data() got a write fault\n");
        return false;
    }
    if (decoded_ins.type != INSTRUCTION_MEMORY) {
        LOG_VMM_ERR("mem_read_set_data() not a memory access instruction\n");
        return false;
    }

    uint64_t *vctx_raw = (uint64_t *)vctx;
    vctx_raw[decoded_ins.decoded.memory_instruction.target_reg] = data;
    return true;
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

static char *vctx_idx_to_name(int vctx_idx)
{
    switch (vctx_idx) {
    case RAX_IDX:
        return "rax";
    case RBX_IDX:
        return "rbx";
    case RCX_IDX:
        return "rcx";
    case RDX_IDX:
        return "rdx";
    case RSI_IDX:
        return "rsi";
    case RDI_IDX:
        return "rdi";
    case RBP_IDX:
        return "rbp";
    case R8_IDX:
        return "r8";
    case R9_IDX:
        return "r9";
    case R10_IDX:
        return "r10";
    case R11_IDX:
        return "r11";
    case R12_IDX:
        return "r12";
    case R13_IDX:
        return "r13";
    case R14_IDX:
        return "r14";
    case R15_IDX:
        return "r15";
    default:
        assert(false);
        return "";
    }
}

void debug_print_instruction(decoded_instruction_ret_t decode_result)
{
    assert(decode_result.len <= X86_MAX_INSTRUCTION_LENGTH);

    LOG_VMM("debug_print_instruction(): decoder result:\n");
    LOG_VMM("instruction length: %d\n", decode_result.len);
    LOG_VMM("raw instruction:\n");
    for (int i = 0; i < decode_result.len; i++) {
        LOG_VMM("0x%hhx\n", decode_result.raw[i]);
    }

    switch (decode_result.type) {
    case INSTRUCTION_DECODE_FAIL:
        LOG_VMM("type: failed to decode\n");
        break;

    case INSTRUCTION_WRITE_IMM:
        LOG_VMM("type: write to memory from immediate\n");
        LOG_VMM("immediate: 0x%lx\n", decode_result.decoded.write_imm_instruction.value);
        LOG_VMM("access width bytes: %d\n", mem_access_width_to_bytes(decode_result));
        break;

    case INSTRUCTION_MEMORY:
        LOG_VMM("type: memory access\n");
        LOG_VMM("zero extend: %d\n", decode_result.decoded.memory_instruction.zero_extend);
        LOG_VMM("target reg: %s\n", vctx_idx_to_name(decode_result.decoded.memory_instruction.target_reg));
        LOG_VMM("access width bytes: %d\n", mem_access_width_to_bytes(decode_result));
        break;

    default:
        LOG_VMM("type: unknown!\n");
        break;
    }
}

decoded_instruction_ret_t decode_instruction(size_t vcpu_id, seL4_Word rip)
{
    uint8_t instruction_buf[X86_MAX_INSTRUCTION_LENGTH];
    uint64_t rip_gpa;
    size_t bytes_remaining;
    assert(gva_to_gpa(vcpu_id, rip, &rip_gpa, &bytes_remaining));

    // @billn fix lazyness, crashes if the instruction crosses a page boundary
    assert(bytes_remaining >= X86_MAX_INSTRUCTION_LENGTH);

    /* Copy 15 bytes of instruction from guest RAM, the actual number of bytes parsed will be less.
     * We have to derive the instruction length ourselves, as the silicon won't tell us... */
    void *instruction_vaddr = gpa_to_vaddr_or_crash(rip_gpa, &bytes_remaining);
    memcpy(instruction_buf, instruction_vaddr, X86_MAX_INSTRUCTION_LENGTH);

    decoded_instruction_ret_t ret = { .type = INSTRUCTION_DECODE_FAIL, .decoded = {} };

    int parsed_byte = 0;
    bool rex_w = false; // 64-bit operand size
    bool rex_r = false; // 4-bit operand, rather than 3-bit
    bool opd_size_override = false;

    /* First step is to match any prefixes, since we are mainly concerned with the `mov` instruction.
     * The only prefixes we care about are the REX and "operand-size override". Since they can appear
     * in any order, parse them in a loop. */
    bool parsing_prefix = true;
    while (parsing_prefix && parsed_byte < X86_MAX_INSTRUCTION_LENGTH) {
        if (instruction_buf[parsed_byte] >> 4 == 4) {
            /* Match for REX byte, which is always 0b0100WRXB */
            uint8_t rex_byte = instruction_buf[parsed_byte];
            parsed_byte += 1;
            rex_w = (rex_byte & BIT(3)) != 0;
            rex_r = (rex_byte & BIT(2)) != 0;
        } else if (instruction_buf[parsed_byte] == 0x66) {
            /* Match for the "operand-size override" prefix, which switch the operand size
             * from 32 to 16 bits, though the REX prefix have more precendence */
            opd_size_override = true;
            parsed_byte += 1;
        } else {
            parsing_prefix = false;
        }
    }

    /* Should still have something left to parse! */
    assert(parsed_byte < X86_MAX_INSTRUCTION_LENGTH);

    /* Match the opcode against a list of known opcodes that we provide decoding logic for.
     * An opcode can be multi-bytes, first match for 1 byte opcode */
    bool opcode_valid = false;
    for (int i = 0; i < NUM_MOV_OPCODES; i++) {
        if (instruction_buf[parsed_byte] == mov_opcodes[i]) {
            opcode_valid = true;
            uint8_t opcode = instruction_buf[parsed_byte];
            parsed_byte++;

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
            break;
        }
    }

    /* no match, now try the 2 bytes movzx opcodes */
    if (!opcode_valid) {
        if (instruction_buf[parsed_byte] == opcode_movzx_byte_from_mem[0]
            && instruction_buf[parsed_byte + 1] == opcode_movzx_byte_from_mem[1]) {
            opcode_valid = true;
            parsed_byte += 2;
            ret.type = INSTRUCTION_MEMORY;
            ret.decoded.memory_instruction.access_width = BYTE_ACCESS_WIDTH;
            ret.decoded.memory_instruction.zero_extend = true;
        } else if (instruction_buf[parsed_byte] == opcode_movzx_word_from_mem[0]
                   && instruction_buf[parsed_byte + 1] == opcode_movzx_word_from_mem[1]) {
            opcode_valid = true;
            parsed_byte += 2;
            ret.type = INSTRUCTION_MEMORY;
            ret.decoded.memory_instruction.access_width = WORD_ACCESS_WIDTH;
            ret.decoded.memory_instruction.zero_extend = true;
        }
    }

    /* no match, try the mov immediate opcode */
    for (int i = 0; i < NUM_MOV_IMM_OPCODES; i++) {
        if (instruction_buf[parsed_byte] == mov_imm_opcodes[i]) {
            opcode_valid = true;
            parsed_byte++;

            assert(guest_in_64_bits());

            ret.type = INSTRUCTION_WRITE_IMM;
            if (rex_w) {
                ret.decoded.write_imm_instruction.access_width = QWORD_ACCESS_WIDTH;
            } else {
                if (opd_size_override) {
                    ret.decoded.write_imm_instruction.access_width = WORD_ACCESS_WIDTH;
                } else {
                    ret.decoded.write_imm_instruction.access_width = DWORD_ACCESS_WIDTH;
                }
            }
            break;
        }
    }

    if (opcode_valid) {
        /* Successfully matched against a known opcode, and `parsed_byte` point to the first byte
         * after the opcode in `instruction_buf`. Now parse the ModR/M, SIB, and Displacement */

        /* "The ModR/M byte encodes a register or an opcode extension, and a register or a memory address"
         *        6           4           0
         * +---+---+---+---+---+---+---+---+
         * |  mod  |    reg    |     rm    |
         * +---+---+---+---+---+---+---+---+
         */
        uint8_t modrm = instruction_buf[parsed_byte];
        uint8_t mod = (modrm >> 6) & 0x3; /* Addressing mode (register vs memory, and displacement size) */
        uint8_t reg = (modrm >> 3) & 0x7; /* Register operand, or opcode extension */
        uint8_t rm = modrm & 0x7; /* Register or memory operand */
        parsed_byte++;

        /* Save the target register now that we have it */
        if (ret.type == INSTRUCTION_MEMORY) {
            ret.decoded.memory_instruction.target_reg = modrm_reg_to_vctx_idx(reg, rex_r);
        }

        bool has_sib = false;
        uint8_t sib = 0;

        /* Parse SIB (Scale-Index-Base)
         * If `mod` != 3 (meaning we are accessing memory, not just a register)
         * AND `rm` == 4 (binary 100), it's an x86 escape code meaning "a SIB byte follows". */
        if (mod != 3 && rm == 4) {
            has_sib = true;
            sib = instruction_buf[parsed_byte];
            parsed_byte++; /* consume SIB */
        }

        /* Calculate Displacement Length
         * Displacement is a constant byte offset added to the memory address. */
        int disp_bytes = 0;
        if (mod == 0) {
            /* mod == 0 usually means no displacement. However, there are two exceptions:
             * 1. rm == 5: In 64-bit, this means RIP-relative addressing.
             * 2. has_sib && base == 5 (sib & 0x7 isolates the SIB base register): Means no base register.
             * Both of these special cases require a mandatory 32-bit (4-byte) displacement. */
            if (rm == 5 || (has_sib && (sib & 0x7) == 5)) {
                disp_bytes = 4;
            }
        } else if (mod == 1) {
            disp_bytes = 1;
        } else if (mod == 2) {
            disp_bytes = 4;
        }

        parsed_byte += disp_bytes;

        /* Parse Immediate (if applicable) */
        if (ret.type == INSTRUCTION_WRITE_IMM) {
            int immediate_bytes = mem_access_width_to_bytes(ret);
            if (immediate_bytes > 4) {
                /* The 0xC7 opcode (MOV to memory/reg) can write up to 64 bits of data,
                 * but the instruction encoding limits the actual immediate value to 32 bits maximum. */
                immediate_bytes = 4;
            }

            memcpy(&(ret.decoded.write_imm_instruction.value), &instruction_buf[parsed_byte], immediate_bytes);

            /* Manual sign extension if REX.W was used with a 32-bit immediate.
             * Because the 0xC7 instruction limits immediates to 32 bits (4 bytes) even when
             * writing to a 64-bit (8 bytes) destination, the CPU sign-extends the value.
             * We must emulate that behavior here. */
            if (immediate_bytes == 4 && mem_access_width_to_bytes(ret) == 8) {
                int32_t signed_imm = (int32_t)ret.decoded.write_imm_instruction.value;
                ret.decoded.write_imm_instruction.value = (uint64_t)signed_imm;
            }

            parsed_byte += immediate_bytes;
        }
    } else {
        LOG_VMM_ERR("failed to decode instruction due to an unknown opcode.\n");
        for (int i = 0; i < X86_MAX_INSTRUCTION_LENGTH; i++) {
            LOG_VMM_ERR("instruction byte: 0x%x\n", instruction_buf[i]);
        }
        vcpu_print_regs(0);
    }

    memcpy(&ret.raw, instruction_vaddr, X86_MAX_INSTRUCTION_LENGTH);
    ret.len = parsed_byte;
    return ret;
}