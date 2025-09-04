/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/fault.h>
#include <libvmm/tcb.h>
#include <libvmm/vcpu.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/riscv/plic.h>
#include <libvmm/arch/riscv/fault.h>
#include <libvmm/arch/riscv/sbi.h>
#include <libvmm/arch/riscv/vcpu.h>

#define BIT(n) (1ul<<(n))
#define PT_SIZE 512
#define GET_PPN(x)      (x & (BIT(44) - 1))
#define VPN_MASK        0x1ff
#define VPN_SHIFT(l)    (12 + 9 * (l))
#define GET_VPN(x, l)   (((x) >> VPN_SHIFT(l)) & VPN_MASK)
#define PPN_SHIFT       12
#define PTE_V           BIT(0)
#define PTE_R           BIT(1)
#define PTE_W           BIT(2)
#define PTE_X           BIT(3)
#define PTE_GET_1G(x)   (((x >> 28) & 0x3ffffff))
#define PTE_GET_2M(x)   (((x >> 19) & (BIT(35) - 1)))
#define PTE_GET_4K(x)   (((x >> 10) & (BIT(44) - 1)))
#define SV39_MODE       0x8

_Static_assert(VPN_SHIFT(2) == 30, "ruh roh");
_Static_assert(VPN_SHIFT(1) == 21, "ruh roh");

static seL4_Word guest_virtual_physical(seL4_Word addr, size_t vcpu_id)
{
    size_t level = 2;
    // for (int i = 0; i < PT_SIZE; i++) {
    //     pt[i] = 0;
    // }

    seL4_Word satp = 0;
    seL4_RISCV_VCPU_ReadRegs_t res = seL4_RISCV_VCPU_ReadRegs(BASE_VCPU_CAP + vcpu_id, seL4_VCPUReg_SATP);
    assert(!res.error);
    satp = res.value;

    uint8_t mode = satp >> 60;
    assert(mode == SV39_MODE);

    seL4_Word ppn = GET_PPN(satp);
    seL4_Word gpa = ppn << PPN_SHIFT;
    uint64_t pte = 0;

    /* Read the guest's page table */
    while (level > 0) {
        // LOG_VMM("copy in %lx level %d\n", gpa, level);
        assert(gpa != 0);
        // for (int i = 0; i < PT_SIZE; i++) {
        //     pt[i] = *(uint64_t *)(gpa + 8 * i);
        // }
        int vpn = GET_VPN(addr, level);
        pte = *(uint64_t *)(gpa + 8 * vpn);
        if (pte & PTE_V && (pte & PTE_R || pte & PTE_X)) {
            /* we reach a leaf page */
            if (level == 2) {
                /* 1 GiB page */
                return (PTE_GET_1G(pte) << 30)  | (addr & (BIT(30) - 1));
            }
            if (level == 1) {
                /* 2 MiB page */
                return (PTE_GET_2M(pte) << 21) | (addr & (BIT(21) - 1));
            }
            if (level == 0) {
                /* 4 KiB page */
                return ((PTE_GET_4K(pte) << 12) | (addr & (BIT(12) - 1)));
            }
        } else {
            assert(pte & PTE_V);
        }
        gpa = (pte >> 10) << 12;
        level--;
    }

    assert(false);
    return 0;
}

extern bool hart_waiting_for_timer[];

bool fault_handle_virtual_inst(size_t vcpu_id, seL4_UserContext *regs, uint32_t inst)
{
    uint8_t op_code = inst & 0x7f;
    uint8_t funct3 = (inst >> 12) & 7;
    uint8_t rd = (inst >> 7) & 0x1f;
    uint8_t rs1 = (inst >> 15) & 0x1f;

    switch (op_code) {
    case OP_CODE_SYSTEM: {
        uint16_t csr = inst >> 20;
        switch (csr) {
        case CSR_STIMECMP: {
            if (funct3 == FUNCT3_CSRRW) {
                uint64_t curr_time = 0;
                asm volatile("rdtime %0" : "=r"(curr_time));
                /* We need to put the source (rs1) into the dest (rd) */
                seL4_Word source = fault_get_reg(regs, rs1);
                // seL4_Word dest = fault_get_reg(regs, rd);
                // LOG_VMM("inst: 0x%lx, curr_time 0x%lx, source: 0x%llx, dest: 0x%llx\n", inst, curr_time, source, dest);
                seL4_RISCV_VCPU_ReadRegs_t res = seL4_RISCV_VCPU_ReadRegs(BASE_VCPU_CAP + vcpu_id, seL4_VCPUReg_SIP);
                assert(!res.error);
                res.value &= ~SIP_TIMER;
                seL4_Error err = seL4_RISCV_VCPU_WriteRegs(BASE_VCPU_CAP + vcpu_id, seL4_VCPUReg_SIP, res.value);
                assert(!err);

                if (curr_time >= source) {
                    inject_timer_irq(vcpu_id);
                } else {
                    // LOG_VMM("waiting for timer\n");
                    seL4_Error err = seL4_RISCV_VCPU_WriteRegs(BASE_VCPU_CAP + vcpu_id, seL4_VCPUReg_TIMER, source);
                    hart_waiting_for_timer[vcpu_id] = true;
                    assert(!err);
                }
                fault_set_reg(regs, rd, source);
                break;
            } else {
                // TODO
                assert(false);
            }
            break;
        }
        default:
            // TODO
            assert(false);
            break;
        }
    }
    break;
    default:
        // TODO
        assert(false);
        break;
    }

    /* Right now we only handle faults from SYSTEM instructions (e.g CSR access),
     * hence no compressed instructions. */
    return fault_advance_vcpu(vcpu_id, regs, false);
}

struct fault_instruction fault_decode_htinst(size_t vcpu_id, uint32_t htinst, seL4_Word addr)
{
    assert(htinst != 0);

    uint32_t instruction;
    if (htinst & (1 << 0)) {
        instruction = htinst | (1 << 1);
    } else if ((htinst & 0x3) == 0) {
        // TODO
        instruction = 0;
        LOG_VMM_ERR("TODO: htinst: 0x%x\n", htinst);
        assert(false);
    } else {
        instruction = htinst;
        assert(false);
    }

    bool compressed = (htinst & 0x3) == 0x1;
    if (!compressed) {
        assert((htinst & 0x3) == 0x3);
    }

    uint8_t op_code = instruction & 0x7f;

    uint8_t funct3 = (htinst >> 12) & 7;

    switch (op_code) {
    case OP_CODE_STORE:
        return (struct fault_instruction) {
            .addr = addr,
            .from_htinst = true,
            .op_code = OP_CODE_STORE,
            .compressed = compressed,
            .funct3 = funct3,
            .rs2 = (instruction >> 20) & (BIT(5) - 1),
        };
    case OP_CODE_LOAD:
        return (struct fault_instruction) {
            .addr = addr,
            .from_htinst = true,
            .op_code = OP_CODE_LOAD,
            .compressed = compressed,
            .funct3 = funct3,
            .rd = (instruction >> 7) & (BIT(5) - 1),
        };
    default:
        LOG_VMM_ERR("htinst invalid op code 0x%x\n", op_code);
        break;
    }

    // assert(funct3 == 2);

    /* TODO: not sure if there's a better way to do this */
    LOG_VMM_ERR("could not decode instruction\n");
    assert(false);

    return (struct fault_instruction) {
        0
    };
}

struct fault_instruction fault_decode_instruction(size_t vcpu_id, seL4_UserContext *regs, seL4_Word htinst,
                                                  seL4_Word addr)
{
    // if (htinst != 0) {
    //     return fault_decode_htinst(vcpu_id, htinst, addr);
    // }

    seL4_Word ip = regs->pc;
    seL4_Word guest_physical = guest_virtual_physical(ip, vcpu_id);
    LOG_VMM("virtual: 0x%lx, guest_physical: 0x%lx\n", ip, guest_physical);
    // TODO: we should assert that the physical address is less than the size of the guest RAM,
    // and within RAM but we do not have enough information to do so right now.

    /*
     * For guest OSes that use the RISC-V 'C' extension we must read the instruction 16-bits at
     * a time, in order to avoid UB since it is valid for *all* instructions, compressed or not,
     * to be aligned to 16-bits.
     */
    uint16_t instruction_lo = *((uint16_t *)guest_physical);
    uint16_t instruction_hi = *(uint16_t *)(guest_physical + 2);
    uint32_t instruction = ((uint32_t)instruction_hi << 16) | instruction_lo;
    uint8_t op_code = instruction & 0x7f;
    /* funct3 is from bits 12:14. */
    uint8_t funct3 = (instruction >> 12) & 0x7;
    // LOG_VMM("decoding fault 0x%lx from 0x%lx, 0x%x, 0x%x\n", addr, ip, instruction, instruction_lo);

    /* If we are in here, we are dealing with a compressed instruction */
    switch (instruction_lo >> 13) {
    case FUNCT3_CSW:
        return (struct fault_instruction) {
            .addr = addr,
            .from_htinst = false,
            .compressed = true,
            .op_code = OP_CODE_STORE,
            .funct3 = FUNCT3_WIDTH_W,
            .rs2 = (instruction_lo >> 2) & (BIT(3) - 1),
        };
    case FUNCT3_CLW:
        return (struct fault_instruction) {
            .addr = addr,
            .from_htinst = false,
            .compressed = true,
            .op_code = OP_CODE_LOAD,
            .funct3 = FUNCT3_WIDTH_W,
            .rd = (instruction_lo >> 2) & (BIT(3) - 1),
        };
    default:
        break;
    }

    LOG_VMM("instruction: 0x%x\n", instruction);
    switch (op_code) {
    case OP_CODE_STORE:
        return (struct fault_instruction) {
            .addr = addr,
            .from_htinst = false,
            .compressed = false,
            .op_code = OP_CODE_STORE,
            .funct3 = funct3,
            .rs2 = (instruction >> 20) & (BIT(5) - 1),
        };
    case OP_CODE_LOAD:
        return (struct fault_instruction) {
            .addr = addr,
            .compressed = false,
            .from_htinst = false,
            .op_code = OP_CODE_LOAD,
            .funct3 = funct3,
            .rd = (instruction >> 7) & (BIT(5) - 1),
        };
    default:
        LOG_VMM_ERR("invalid op code 0x%x\n", op_code);
        break;
    }

    LOG_VMM_ERR("could not decode instruction at PC: 0x%lx\n", ip);
    assert(false);

    return (struct fault_instruction) {
        0
    };
}

/* This global is valid anytime we are currently handling a virtual memory fault. */
fault_instruction_t decoded_instruction;

char *fault_to_string(seL4_Word fault_label)
{
    switch (fault_label) {
    case seL4_Fault_VMFault:
        return "virtual memory exception";
    case seL4_Fault_UnknownSyscall:
        return "unknown syscall";
    case seL4_Fault_UserException:
        return "user exception";
    case seL4_Fault_VCPUFault:
        return "vCPU";
    default:
        return "unknown fault";
    }
}

bool fault_is_read(seL4_Word fsr)
{
    return fsr == TRAP_LOAD_ACCESS;
}

bool fault_is_write(seL4_Word fsr)
{
    return fsr == TRAP_STORE_ACCESS;
}

void fault_emulate_read_access(fault_instruction_t *instruction, seL4_UserContext *regs, uint32_t data)
{
    seL4_Word reg;
    if (!instruction->from_htinst && instruction->compressed) {
        reg = fault_get_reg_compressed(regs, instruction->rd);
    } else {
        reg = fault_get_reg(regs, instruction->rd);
    }

    /* Depending on the type of load/store, we have to either sign-extend or zero-extend. */
    switch (instruction->funct3) {
    case FUNCT3_WIDTH_B:
        /* 8-bit value from memory, then sign-extends to 32-bits */
        reg |= 0xffffff00;
        reg &= 0xffffff00;
        break;
    case FUNCT3_WIDTH_BU:
    case FUNCT3_WIDTH_HU:
    case FUNCT3_WIDTH_W:
        /* All 32-bits should be zero since either we are loading a whole word
         * or zero-extending to 32-bits. */
        reg &= 0x00000000;
        break;
    case FUNCT3_WIDTH_H:
        /* First we bitwise OR to sign-extend to 32-bits, then make sure the
         * least-significant 16-bits are zero for the data. */
        reg |= 0xffff0000;
        reg &= 0xffff0000;
        break;
    default:
        LOG_VMM_ERR("invalid funct3 (0x%hhx) at PC 0x%lx\n", instruction->funct3, regs->pc);
        assert(false);
        break;
    }

    /* If the faulting address is not 4-byte aligned then we manipulate
     * the data to get the right part of it into the resulting register. */
    uint8_t data_offset = (instruction->addr & 0x3) * 8;
    reg |= data >> data_offset;

    if (!instruction->from_htinst && instruction->compressed) {
        fault_set_reg_compressed(regs, instruction->rd, reg);
    } else {
        fault_set_reg(regs, instruction->rd, reg);
    }
}

uint32_t fault_instruction_data(fault_instruction_t *instruction, seL4_UserContext *regs)
{
    uint32_t reg;
    if (!instruction->from_htinst && instruction->compressed) {
        reg = fault_get_reg_compressed(regs, instruction->rs2);
    } else {
        reg = fault_get_reg(regs, instruction->rs2);
    }

    return reg;
}

void fault_instruction_dump(fault_instruction_t *instruction, seL4_UserContext *regs) {
    LOG_VMM("decoded instruction at PC 0x%lx:\n", regs->pc);
    LOG_VMM("   fault addr: 0x%lx\n", instruction->addr);
    LOG_VMM("   derived from htinst: %s\n", instruction->from_htinst ? "true" : "false");
    LOG_VMM("   compressed: %s\n", instruction->compressed ? "true" : "false");
    LOG_VMM("   funct3: 0x%hu\n", instruction->funct3);
    LOG_VMM("   operation: ");
    switch (instruction->op_code) {
    case OP_CODE_STORE:
        printf("store");
        break;
    case OP_CODE_LOAD:
        printf("load");
        break;
    case OP_CODE_SYSTEM:
        printf("system register access");
        break;
    default:
        printf("<unknown op code>");
    }
    printf("\n");
    if (instruction->op_code == OP_CODE_STORE) {
        LOG_VMM("   rs2: 0x%hu\n", instruction->rs2);
    } else if (instruction->op_code == OP_CODE_LOAD) {
        LOG_VMM("   rd: 0x%hu\n", instruction->rd);
    }
}

void fault_emulate_write_access(fault_instruction_t *instruction, seL4_UserContext *regs, uint32_t *value, uint32_t write_data) {
    uint32_t data = write_data;
    uint8_t offset = (instruction->addr & 0x3) * 8;
    uint32_t persistent_data_mask = 0;
    switch (instruction->funct3) {
    case FUNCT3_WIDTH_B:
        persistent_data_mask = ~(0xff << offset);
        data &= 0x000000ff;
        break;
    case FUNCT3_WIDTH_H:
        /* TODO: potential UB if there's something like a write-half at offset 0x3. I don't
         * even know if that's legal or how the tf we would handle it. */
        persistent_data_mask = ~(0xffff << offset);
        data &= 0x0000ffff;
        break;
    case FUNCT3_WIDTH_W:
        persistent_data_mask = 0;
        data &= 0xffffffff;
        break;
    default:
        LOG_VMM_ERR("invalid funct3 (0x%hhx) at PC 0x%lx\n", instruction->funct3, regs->pc);
        assert(false);
        break;
    }

    /* Now we need to OR the write data with the bits of the current value we are not writing
     * to, depending on the data-width of the instruction. */

    /* Finally write out the constructed data */
    uint32_t persistent_data = *value & persistent_data_mask;
    *value = (persistent_data | data);
}

bool fault_advance_vcpu(size_t vcpu_id, seL4_UserContext *regs, bool compressed)
{
    if (compressed) {
        regs->pc += 2;
    } else {
        regs->pc += 4;
    }
    /*
     * Do not explicitly resume the TCB because we will eventually reply to the
     * fault which will result in the TCB being restarted.
     */
    seL4_Error err = seL4_TCB_WriteRegisters(BASE_VM_TCB_CAP + vcpu_id, false, 0, SEL4_USER_CONTEXT_SIZE, regs);
    assert(err == seL4_NoError);

    return (err == seL4_NoError);
}

bool fault_handle(size_t vcpu_id, microkit_msginfo msginfo)
{
    size_t label = microkit_msginfo_get_label(msginfo);
    bool success = false;
    switch (label) {
    case seL4_Fault_VMFault: {
        return fault_handle_vm_exception(vcpu_id);
    }
    case seL4_Fault_VCPUFault: {
        seL4_Word cause = seL4_GetMR(seL4_VCPUFault_Cause);
        seL4_UserContext regs;
        // TODO: potentially reading TCB registers twice with VM fault handling code
        seL4_Error err = seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + vcpu_id, false, 0, SEL4_USER_CONTEXT_SIZE, &regs);
        assert(err == seL4_NoError);
        if (cause == TRAP_ENV_CALL_VS_MODE) {
            return fault_handle_sbi(vcpu_id, &regs);
        } else if (cause == TRAP_VIRTUAL_INSTRUCTION) {
            uint32_t inst = seL4_GetMR(seL4_VCPUFault_Data);
            success = fault_handle_virtual_inst(vcpu_id, &regs, inst);
            if (!success) {
                LOG_VMM_ERR("unable to handle vCPU virtual instruction fault, instruction: 0x%lx, PC: 0x%lx\n", inst, regs.pc);
            }
        } else {
            LOG_VMM_ERR("unhandled vCPU fault cause: 0x%lx\n", seL4_GetMR(seL4_VCPUFault_Cause));
        }
        break;
    }
    default:
        /* We have reached a genuinely unexpected case, stop the guest. */
        LOG_VMM_ERR("unknown fault label 0x%lx, stopping guest with ID 0x%lx\n", label, vcpu_id);
        microkit_vcpu_stop(vcpu_id);
        break;
    }

    if (!success) {
        /* Dump the TCB and vCPU registers to hopefully get information as
         * to what has gone wrong. */
        tcb_print_regs(vcpu_id);
        vcpu_print_regs(vcpu_id);
        LOG_VMM_ERR("Failed to handle %s fault\n", fault_to_string(label));
    }

    return success;
}
