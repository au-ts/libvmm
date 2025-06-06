/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/aarch64/fault.h>
#include <libvmm/arch/aarch64/vgic/vgic.h>


static seL4_Word wzr = 0;
static seL4_Word *decode_rt(size_t reg_idx, seL4_UserContext *regs)
{
    /*
     * This switch statement is necessary due to a mismatch between how
     * seL4 orders the TCB registers compared to what the architecture
     * encodes the Syndrome Register transfer.
     */
    switch (reg_idx) {
    case 0:
        return &regs->x0;
    case 1:
        return &regs->x1;
    case 2:
        return &regs->x2;
    case 3:
        return &regs->x3;
    case 4:
        return &regs->x4;
    case 5:
        return &regs->x5;
    case 6:
        return &regs->x6;
    case 7:
        return &regs->x7;
    case 8:
        return &regs->x8;
    case 9:
        return &regs->x9;
    case 10:
        return &regs->x10;
    case 11:
        return &regs->x11;
    case 12:
        return &regs->x12;
    case 13:
        return &regs->x13;
    case 14:
        return &regs->x14;
    case 15:
        return &regs->x15;
    case 16:
        return &regs->x16;
    case 17:
        return &regs->x17;
    case 18:
        return &regs->x18;
    case 19:
        return &regs->x19;
    case 20:
        return &regs->x20;
    case 21:
        return &regs->x21;
    case 22:
        return &regs->x22;
    case 23:
        return &regs->x23;
    case 24:
        return &regs->x24;
    case 25:
        return &regs->x25;
    case 26:
        return &regs->x26;
    case 27:
        return &regs->x27;
    case 28:
        return &regs->x28;
    case 29:
        return &regs->x29;
    case 30:
        return &regs->x30;
    case 31:
        return &wzr;
    default:
        LOG_VMM_ERR("failed to decode Rt, attempted to access invalid register index 0x%lx\n", reg_idx);
        return NULL;
    }
}

bool handle_sysreg_64_fault(size_t vcpu_id, uint32_t hsr, seL4_UserContext *regs) {
    // @billn this code is a bit stupid, todo fix
    uint32_t iss = hsr & 0xffffff;
    uint8_t op0 = (iss >> 20) & 0x3;
    uint8_t op2 = (iss >> 17) & 0x7;
    uint8_t op1 = (iss >> 14) & 0x7;
    uint8_t crn = (iss >> 10) & 0xf;
    uint8_t crm = (iss >> 1) & 0xf;
    bool is_read = iss & 0x1;

    if (op0 == 3 && op2 == 5 && op1 == 0 && crn == 12 && crm == 11 && !is_read) {
        // LOG_VMM("trapped a write to ICC_SGI1R_EL1 from vcpu %lu\n", vcpu_id);
        
        // @billn hack, should be using fault get data!
        size_t reg_idx = (hsr >> 5) & 0x1f;
        size_t data = *decode_rt(reg_idx, regs);

        // LOG_VMM("with data 0x%lx\n", data);

        uint8_t intid = (data >> 24) & 0xf;
        for (int i = 0; i < GUEST_NUM_VCPUS; i++) {
            // check the target list and raise IRQ on the approproate vCPUs.
            // @billn double check: make sure IRM bit is honoured.
            // @billn double check: affinity routing?
            if (data >> i & 0x1) {
                assert(vgic_inject_irq(i, intid));
            }
        }
        return fault_advance_vcpu(vcpu_id, regs);
    }

    LOG_VMM("trapped sysreg 64 exception with unknown instruction: op0 %u, op2 %u, op1 %u, crn %u, crm %u, is read %u\n", op0, op2, op1, crn, crm, is_read);
    return false;
}