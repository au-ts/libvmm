/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <microkit.h>

enum fault_op_code {
    OP_CODE_STORE = 0b0100011,
    OP_CODE_LOAD = 0b0000011,
    OP_CODE_SYSTEM = 0b01110011,
};

typedef struct fault_instruction {
    seL4_Word addr;
    /* Need to know whether this instruction was decoded from the value of htinst
     * for handling compressed instructions. */
    bool from_htinst;
    bool compressed;
    uint8_t op_code;
    uint8_t funct3;
    union {
        /* Valid when the operation is a store */
        uint8_t rs2;
        /* Valid when the operation is a load */
        uint8_t rd;
    };
} fault_instruction_t;

#define FUNCT3_CSW 0b110
#define FUNCT3_CLW 0b010
#define FUNCT3_CSRRW 0b001

/* Corresponds to lb or sb */
#define FUNCT3_WIDTH_B  0b000
/* Corresponds to lbu */
#define FUNCT3_WIDTH_BU 0b100
/* Corresponds to lh or sh */
#define FUNCT3_WIDTH_H  0b001
/* Corresonds to lhu */
#define FUNCT3_WIDTH_HU 0b101
/* Corresponds to lw or sw */
#define FUNCT3_WIDTH_W  0b010

/* CSR 'stimecmp', for emulating the 'Sstc' extension. */
#define CSR_STIMECMP 0x14D

enum trap_cause {
    TRAP_ILLEGAL_INSTRUCTION = 2,
    TRAP_ENV_CALL_VS_MODE = 10,
    TRAP_LOAD_ACCESS = 21,
    TRAP_VIRTUAL_INSTRUCTION = 22,
    TRAP_STORE_ACCESS = 23,
};

/* Fault-handling functions */
bool fault_handle(size_t vcpu_id, microkit_msginfo msginfo);

void fault_emulate_read_access(fault_instruction_t *instruction, seL4_UserContext *regs, uint32_t data);
uint32_t fault_instruction_data(fault_instruction_t *instruction, seL4_UserContext *regs);
void fault_emulate_write_access(fault_instruction_t *instruction, seL4_UserContext *regs, uint32_t *value, uint32_t write_data);

/* Calls seL4_TCB_WriteRegisters after incrementing the program counter. Incremenets by 2 if
 * we are dealing with a 'compressed' instruction and 4 if not. */
bool fault_advance_vcpu(size_t vcpu_id, seL4_UserContext *regs, bool compressed);

bool fault_is_write(seL4_Word fsr);
bool fault_is_read(seL4_Word fsr);

struct fault_instruction fault_decode_instruction(size_t vcpu_id, seL4_UserContext *regs, seL4_Word ip, seL4_Word addr);

static inline seL4_Word fault_get_reg(seL4_UserContext *regs, int index)
{
    switch (index) {
    // X0 is always ZERO
    case 0:
        return 0;
    case 1:
        return regs->ra;
    case 2:
        return regs->sp;
    case 3:
        return regs->gp;
    case 4:
        return regs->tp;
    case 5:
        return regs->t0;
    case 6:
        return regs->t1;
    case 7:
        return regs->t2;
    case 8:
        return regs->s0;
    case 9:
        return regs->s1;
    case 10:
        return regs->a0;
    case 11:
        return regs->a1;
    case 12:
        return regs->a2;
    case 13:
        return regs->a3;
    case 14:
        return regs->a4;
    case 15:
        return regs->a5;
    case 16:
        return regs->a6;
    case 17:
        return regs->a7;
    case 18:
        return regs->s2;
    case 19:
        return regs->s3;
    case 20:
        return regs->s4;
    case 21:
        return regs->s5;
    case 22:
        return regs->s6;
    case 23:
        return regs->s7;
    case 24:
        return regs->s8;
    case 25:
        return regs->s9;
    case 26:
        return regs->s10;
    case 27:
        return regs->s11;
    case 28:
        return regs->t3;
    case 29:
        return regs->t4;
    case 30:
        return regs->t5;
    case 31:
        return regs->t6;
    default:
        LOG_VMM_ERR("Invalid index %d\n", index);
        assert(false);
        return -1;
    }
}

static inline seL4_Word fault_get_reg_compressed(seL4_UserContext *regs, int index)
{
    switch (index) {
    case 0:
        return regs->s0;
    case 1:
        return regs->s1;
    case 2:
        return regs->a0;
    case 3:
        return regs->a1;
    case 4:
        return regs->a2;
    case 5:
        return regs->a3;
    case 6:
        return regs->a4;
    case 7:
        return regs->a5;
    default:
        LOG_VMM_ERR("invalid compressed register index %d\n", index);
        assert(false);
        return -1;
    }
}

static inline void fault_set_reg_compressed(seL4_UserContext *regs, int index, seL4_Word value)
{
    switch (index) {
    case 0:
        regs->s0 = value;
        break;
    case 1:
        regs->s1 = value;
        break;
    case 2:
        regs->a0 = value;
        break;
    case 3:
        regs->a1 = value;
        break;
    case 4:
        regs->a2 = value;
        break;
    case 5:
        regs->a3 = value;
        break;
    case 6:
        regs->a4 = value;
        break;
    case 7:
        regs->a5 = value;
        break;
    default:
        LOG_VMM_ERR("invalid compressed register index %d\n", index);
        assert(false);
    }
}

static inline void fault_set_reg(seL4_UserContext *regs, int index, seL4_Word v)
{
    switch (index) {
    // X0 is always ZERO
    case 0:
        return;
    case 1:
        regs->ra = v;
        return;
    case 2:
        regs->sp = v;
        return;
    case 3:
        regs->gp = v;
        return;
    case 4:
        regs->tp = v;
        return;
    case 5:
        regs->t0 = v;
        return;
    case 6:
        regs->t1 = v;
        return;
    case 7:
        regs->t2 = v;
        return;
    case 8:
        regs->s0 = v;
        return;
    case 9:
        regs->s1 = v;
        return;
    case 10:
        regs->a0 = v;
        return;
    case 11:
        regs->a1 = v;
        return;
    case 12:
        regs->a2 = v;
        return;
    case 13:
        regs->a3 = v;
        return;
    case 14:
        regs->a4 = v;
        return;
    case 15:
        regs->a5 = v;
        return;
    case 16:
        regs->a6 = v;
        return;
    case 17:
        regs->a7 = v;
        return;
    case 18:
        regs->s2 = v;
        return;
    case 19:
        regs->s3 = v;
        return;
    case 20:
        regs->s4 = v;
        return;
    case 21:
        regs->s5 = v;
        return;
    case 22:
        regs->s6 = v;
        return;
    case 23:
        regs->s7 = v;
        return;
    case 24:
        regs->s8 = v;
        return;
    case 25:
        regs->s9 = v;
        return;
    case 26:
        regs->s10 = v;
        return;
    case 27:
        regs->s11 = v;
        return;
    case 28:
        regs->t3 = v;
        return;
    case 29:
        regs->t4 = v;
        return;
    case 30:
        regs->t5 = v;
        return;
    case 31:
        regs->t6 = v;
        return;
    default:
        LOG_VMM_ERR("Invalid index %d\n", index);
        assert(false);
    }
}
