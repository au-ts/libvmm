/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/fault.h>
#include <libvmm/tcb.h>
#include <libvmm/vcpu.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/riscv/plic.h>
#include <libvmm/arch/riscv/fault.h>
#include <libvmm/arch/riscv/sbi.h>

#define WFI_INST    0x10500073

enum trap_cause {
    TRAP_ILLEGAL_INSTRUCTION = 2,
    TRAP_ENV_CALL_VS_MODE = 10,
    TRAP_VIRTUAL_INSTRUCTION = 22,
};

#define BIT(n) (1ul<<(n))
#define PT_SIZE 512
#define GET_PPN(x)      (x & 0xfffffffffff)
#define VPN_MASK        0x1ff
#define VPN_SHIFT(l)    (12 + 9 * (l))
#define GET_VPN(x, l)   (((x) >> VPN_SHIFT(l)) & VPN_MASK)
#define PPN_SHIFT       12
#define PTE_V           BIT(0)
#define PTE_R           BIT(1)
#define PTE_W           BIT(2)
#define PTE_X           BIT(3)
#define PTE_GET_1G(x)   (((x >> 28) & 0x3ffffff))
#define PTE_GET_2M(x)   (((x >> 19) & (BIT(36) - 1)))
#define PTE_GET_4K(x)   (((x >> 10) & (BIT(45) - 1)))
#define SV39_MODE       0x8

static uint64_t pt[512];

static seL4_Word guest_virtual_physical(seL4_Word addr, size_t vcpu_id) {
    size_t level = 2;
    // uint64_t *ppt = &pt;
    for (int i = 0; i < PT_SIZE; i++) {
        pt[i] = 0;
    }

    seL4_Word satp = 0;
    seL4_RISCV_VCPU_ReadRegs_t res = seL4_RISCV_VCPU_ReadRegs(BASE_VCPU_CAP + vcpu_id, seL4_VCPUReg_SATP);
    assert(!res.error);
    satp = res.value;
    // LOG_VMM("satp: 0x%lx\n", satp);
    uint8_t mode = satp >> 60;
    // LOG_VMM("mode is 0x%lx\n", mode);

    assert(mode == SV39_MODE);
    // switch mode {
    // case SV39_MODE:
    // case
    // }

    seL4_Word ppn = GET_PPN(satp);
    seL4_Word gpa = ppn << PPN_SHIFT;
    uint64_t pte = 0;

    /* Read the guest's page table */
    while (level > 0) {
        // LOG_VMM("copy in %lx level %d\n", gpa, level);
        assert(gpa != 0);
        for (int i = 0; i < PT_SIZE; i++) {
            pt[i] = *(uint64_t *)(gpa + 8 * i);
        }
        int vpn = GET_VPN(addr, level);
        pte = pt[vpn];
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

        }
        gpa = (pte >> 10) << 12;
        level--;
    }

    assert(false);
    return 0;
}

#define FUNCT3_CSW 0b110
#define FUNCT3_CLW 0b010

struct fault_instruction fault_decode_htinst(size_t vcpu_id, uint32_t htinst) {
    uint32_t instruction;
    if (htinst & 0x1) {
        instruction = htinst | 0x2;
    } else if ((htinst & 0x3) == 0) {
        // TODO
        instruction = 0;
        LOG_VMM_ERR("TODO: htinst: 0x%x\n", htinst);
        assert(false);
    } else {
        instruction = htinst;
    }

    bool compressed = (htinst & 0x3) == 0x1;
    uint8_t width = (compressed) ? 2 : 4;
    // LOG_VMM("htinst 0x%x -> instruction 0x%x, compressed: %d\n", htinst, instruction, compressed);

    // TODO: check this.
    uint8_t op_code = instruction & 0x7f;

    switch (op_code) {
    case OP_CODE_STORE:
        return (struct fault_instruction){
            .from_htinst = true,
            .op_code = OP_CODE_STORE,
            .width = width,
            .rs2 = (instruction >> 20) & (BIT(5) - 1),
        };
    case OP_CODE_LOAD:
        return (struct fault_instruction){
            .from_htinst = true,
            .op_code = OP_CODE_LOAD,
            .width = width,
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

    return (struct fault_instruction){ 0 };
}

// TODO: we need to have some kind of 'guest' struct that we instead pass around or populate upon init.
extern uintptr_t guest_ram_vaddr;

struct fault_instruction fault_decode_instruction(size_t vcpu_id, seL4_UserContext *regs, seL4_Word htinst) {
    if (htinst != 0) {
        // LOG_VMM("decoding fault at 0x%lx, from htinst: 0x%lx\n", regs->pc, htinst);
        return fault_decode_htinst(vcpu_id, htinst);
    }

    seL4_Word ip = regs->pc;
    LOG_VMM("decoding at ip 0x%lx\n", ip);
    seL4_Word guest_physical = guest_virtual_physical(ip, vcpu_id);
    assert(guest_physical >= guest_ram_vaddr && guest_physical <= guest_ram_vaddr + 0x10000000);
    // LOG_VMM("guest_physical: 0x%lx\n", guest_physical);
    /*
     * For guest OSes that use the RISC-V 'C' extension we must read the instruction 16-bits at
     * a time, in order to avoid UB since it is valid for *all* instructions, compressed or not,
     * to be aligned to 16-bits.
     */
    uint16_t instruction_lo = *((uint16_t *)guest_physical);
    uint16_t instruction_hi = *(uint16_t *)(guest_physical + 16);
    uint32_t instruction = ((uint32_t)instruction_hi << 16) | instruction_lo;
    // LOG_VMM("instruction is 0x%lx\n", instruction);
    LOG_VMM("guest_physical: 0x%lx, instruction: 0x%x, instruction_lo: 0x%x, instruction_hi: 0x%x\n", guest_physical, instruction, instruction_lo, instruction_hi);
    // TODO: check this.
    uint8_t op_code = instruction & 0x7f;
    /* funct3 is from bits 12:14. */
    // uint8_t funct3 = (instruction >> 12) & 0x7;

    // uint8_t compressed_funct3 = (funct3 >> 2) | (op_code & 0b11);
    /* Make sure data width is word size. TODO: don't know if we need to have this
     * restriction or how Yanyan found out the data width numbers for funct3. */

    /* If we are in here, we are dealing with a compressed instruction */
    switch (instruction_lo >> 13) {
        case FUNCT3_CSW:
            return (struct fault_instruction){
                .from_htinst = false,
                .op_code = OP_CODE_STORE,
                .width = 2,
                .rs2 = (instruction_lo >> 2) & (BIT(3) - 1),
            };
        case FUNCT3_CLW:
            return (struct fault_instruction){
                .from_htinst = false,
                .op_code = OP_CODE_LOAD,
                .width = 2,
                .rd = (instruction_lo >> 2) & (BIT(3) - 1),
            };
        default:
            break;
    }

    switch (op_code) {
    case OP_CODE_STORE:
        return (struct fault_instruction){
            .op_code = OP_CODE_STORE,
            .width = 4,
            .rs2 = (instruction >> 20) & (BIT(5) - 1),
        };
    case OP_CODE_LOAD:
        return (struct fault_instruction){
            .op_code = OP_CODE_LOAD,
            .width = 4,
            .rd = (instruction >> 7) & (BIT(5) - 1),
        };
    default:
        LOG_VMM_ERR("invalid op code 0x%x\n", op_code);
        break;
    }

    // assert(funct3 == 2);

    /* TODO: not sure if there's a better way to do this */
    LOG_VMM_ERR("could not decode instruction at PC: 0x%lx\n", ip);
    assert(false);

    return (struct fault_instruction){ 0 };
}

/* This global is valid anytime we are currently handling a virtual memory fault. */
fault_instruction_t decoded_instruction;

char *fault_to_string(seL4_Word fault_label) {
    // TODO
    switch (fault_label) {
    case seL4_Fault_VMFault: return "virtual memory exception";
    case seL4_Fault_UnknownSyscall: return "unknown syscall";
    case seL4_Fault_UserException: return "user exception";
    case seL4_Fault_VCPUFault: return "vCPU";
    default: return "unknown fault";
    }
}

#define VSCAUSE_LOAD_ACCESS_FAULT (21)
#define VSCAUSE_STORE_ACCESS_FAULT (23)

bool fault_is_read(seL4_Word fsr) {
    return fsr == VSCAUSE_LOAD_ACCESS_FAULT;
}

bool fault_is_write(seL4_Word fsr) {
    return fsr == VSCAUSE_STORE_ACCESS_FAULT;
}

void fault_emulate_read_access(fault_instruction_t *instruction, seL4_UserContext *regs, uint32_t data) {
    assert(instruction->width == 2 || instruction->width == 4);

    /* TODO: revisit */
    seL4_Word reg;
    if (!instruction->from_htinst && instruction->width == 2) {
        reg = fault_get_reg_compressed(regs, instruction->rd);
    } else {
        reg = fault_get_reg(regs, instruction->rd);
    }

    reg &= 0xffffffff00000000;
    reg |= data;
    if (!instruction->from_htinst && instruction->width == 2) {
        fault_set_reg_compressed(regs, instruction->rd, reg);
    } else {
        fault_set_reg(regs, instruction->rd, reg);
    }
}

uint32_t fault_instruction_data(fault_instruction_t *instruction, seL4_UserContext *regs) {
    assert(instruction->width == 2 || instruction->width == 4);
    if (!instruction->from_htinst && instruction->width == 2) {
        return fault_get_reg_compressed(regs, instruction->rs2);
    } else {
        return fault_get_reg(regs, instruction->rs2);
    }
}


bool fault_advance_vcpu(size_t vcpu_id, seL4_UserContext *regs, fault_instruction_t *instruction) {
    assert(instruction->width == 2 || instruction->width == 4);
    regs->pc += instruction->width;
    /*
     * Do not explicitly resume the TCB because we will eventually reply to the
     * fault which will result in the TCB being restarted.
     */
    seL4_Error err = seL4_TCB_WriteRegisters(BASE_VM_TCB_CAP + vcpu_id, false, 0, SEL4_USER_CONTEXT_SIZE, regs);
    assert(err == seL4_NoError);

    return (err == seL4_NoError);
}

bool fault_handle(size_t vcpu_id, microkit_msginfo msginfo) {
    size_t label = microkit_msginfo_get_label(msginfo);
    // LOG_VMM("handling fault '%s'\n", fault_to_string(label));
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
                uint32_t data = seL4_GetMR(seL4_VCPUFault_Data);
                // if (data == WFI_INST) {
                    // TODO: handle WFI properly
                    // regs.pc += 4;
                    // seL4_TCB_WriteRegisters(BASE_VM_TCB_CAP + vcpu_id, false, 0, sizeof(seL4_UserContext) / sizeof(seL4_Word), &regs);
                    // return true;
                // } else {
                LOG_VMM_ERR("unknown vCPU virtual instruction fault, data: 0x%lx, pc: 0x%lx\n", data, regs.pc);
                // }
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
