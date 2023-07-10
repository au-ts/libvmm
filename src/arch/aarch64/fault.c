/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "hsr.h"
#include "../../util/util.h"
#include "smc.h"
#include "vgic/vgic.h"
#include "tcb.h"
#include "vcpu.h"
#include "fault.h"

// #define CPSR_THUMB                 (1 << 5)
// #define CPSR_IS_THUMB(x)           ((x) & CPSR_THUMB)

// int fault_is_32bit_instruction(seL4_UserContext *regs)
// {
//     // @ivanv: assuming VCPU fault
//     return !CPSR_IS_THUMB(regs->spsr);
// }

bool fault_advance_vcpu(size_t vcpu_id, seL4_UserContext *regs) {
    // For now we just ignore it and continue
    // Assume 64-bit instruction
    regs->pc += 4;
    int err = seL4_TCB_WriteRegisters(BASE_VM_TCB_CAP + vcpu_id, true, 0, SEL4_USER_CONTEXT_SIZE, regs);
    assert(err == seL4_NoError);

    return (err == seL4_NoError);
}

char *fault_to_string(seL4_Word fault_label) {
    switch (fault_label) {
        case seL4_Fault_VMFault: return "virtual memory";
        case seL4_Fault_UnknownSyscall: return "unknown syscall";
        case seL4_Fault_UserException: return "user exception";
        case seL4_Fault_VGICMaintenance: return "VGIC maintenance";
        case seL4_Fault_VCPUFault: return "VCPU fault";
        case seL4_Fault_VPPIEvent: return "VPPI event";
        default: return "unknown fault";
    }
}

enum fault_width {
    WIDTH_DOUBLEWORD = 0,
    WIDTH_WORD = 1,
    WIDTH_HALFWORD = 2,
    WIDTH_BYTE = 3,
};

static enum fault_width fault_get_width(uint64_t fsr)
{
    if (HSR_IS_SYNDROME_VALID(fsr)) {
        switch (HSR_SYNDROME_WIDTH(fsr)) {
            case 0: return WIDTH_BYTE;
            case 1: return WIDTH_HALFWORD;
            case 2: return WIDTH_WORD;
            case 3: return WIDTH_DOUBLEWORD;
            default:
                // @ivanv: reviist
                // print_fault(f);
                assert(0);
                return 0;
        }
    } else {
        LOG_VMM_ERR("Received invalid FSR: 0x%lx\n", fsr);
        // @ivanv: reviist
        // int rt;
        // rt = decode_instruction(f);
        // assert(rt >= 0);
        return -1;
    }
}

uint64_t fault_get_data_mask(uint64_t addr, uint64_t fsr)
{
    uint64_t mask = 0;
    switch (fault_get_width(fsr)) {
        case WIDTH_BYTE:
            mask = 0x000000ff;
            assert(!(addr & 0x0));
            break;
        case WIDTH_HALFWORD:
            mask = 0x0000ffff;
            assert(!(addr & 0x1));
            break;
        case WIDTH_WORD:
            mask = 0xffffffff;
            assert(!(addr & 0x3));
            break;
        case WIDTH_DOUBLEWORD:
            mask = ~mask;
            break;
        default:
            LOG_VMM_ERR("unknown width: 0x%lx, from FSR: 0x%lx, addr: 0x%lx\n",
                fault_get_width(fsr), fsr, addr);
            assert(0);
            return 0;
    }
    mask <<= (addr & 0x3) * 8;
    return mask;
}

static uint64_t wzr = 0;
uint64_t *decode_rt(uint64_t reg, seL4_UserContext *regs)
{
    switch (reg) {
        case 0: return &regs->x0;
        case 1: return &regs->x1;
        case 2: return &regs->x2;
        case 3: return &regs->x3;
        case 4: return &regs->x4;
        case 5: return &regs->x5;
        case 6: return &regs->x6;
        case 7: return &regs->x7;
        case 8: return &regs->x8;
        case 9: return &regs->x9;
        case 10: return &regs->x10;
        case 11: return &regs->x11;
        case 12: return &regs->x12;
        case 13: return &regs->x13;
        case 14: return &regs->x14;
        case 15: return &regs->x15;
        case 16: return &regs->x16;
        case 17: return &regs->x17;
        case 18: return &regs->x18;
        case 19: return &regs->x19;
        case 20: return &regs->x20;
        case 21: return &regs->x21;
        case 22: return &regs->x22;
        case 23: return &regs->x23;
        case 24: return &regs->x24;
        case 25: return &regs->x25;
        case 26: return &regs->x26;
        case 27: return &regs->x27;
        case 28: return &regs->x28;
        case 29: return &regs->x29;
        case 30: return &regs->x30;
        case 31: return &wzr;
        default:
            printf("invalid reg %d\n", reg);
            assert(!"Invalid register");
            return NULL;
    }
}

bool fault_is_write(uint64_t fsr)
{
    return (fsr & (1U << 6)) != 0;
}

bool fault_is_read(uint64_t fsr)
{
    return !fault_is_write(fsr);
}

static int get_rt(uint64_t fsr)
{

    int rt = -1;
    if (HSR_IS_SYNDROME_VALID(fsr)) {
        rt = HSR_SYNDROME_RT(fsr);
    } else {
        printf("decode_insturction for arm64 not implemented\n");
        assert(0);
        // @ivanv: implement decode instruction for aarch64
        // rt = decode_instruction(f);
    }
    assert(rt >= 0);
    return rt;
}

uint64_t fault_get_data(seL4_UserContext *regs, uint64_t fsr)
{
    /* Get register opearand */
    int rt  = get_rt(fsr);

    uint64_t data = *decode_rt(rt, regs);

    return data;
}

uint64_t fault_emulate(seL4_UserContext *regs, uint64_t reg, uint64_t addr, uint64_t fsr, uint64_t reg_val)
{
    uint64_t m, s;
    s = (addr & 0x3) * 8;
    m = fault_get_data_mask(addr, fsr);
    if (fault_is_read(fsr)) {
        /* Read data must be shifted to lsb */
        return (reg & ~(m >> s)) | ((reg_val & m) >> s);
    } else {
        /* Data to write must be shifted left to compensate for alignment */
        return (reg & ~m) | ((reg_val << s) & m);
    }
}

bool fault_advance(size_t vcpu_id, seL4_UserContext *regs, uint64_t addr, uint64_t fsr, uint64_t reg_val)
{
    /* Get register opearand */
    int rt = get_rt(fsr);

    uint64_t *reg_ctx = decode_rt(rt, regs);
    *reg_ctx = fault_emulate(regs, *reg_ctx, addr, fsr, reg_val);
    // DFAULT("%s: Emulate fault @ 0x%x from PC 0x%x\n",
    //        fault->vcpu->vm->vm_name, fault->addr, fault->ip);

    return fault_advance_vcpu(vcpu_id, regs);
}

bool handle_vcpu_fault(size_t vcpu_id)
{
    uint32_t hsr = sel4cp_mr_get(seL4_VCPUFault_HSR);
    uint64_t hsr_ec_class = HSR_EXCEPTION_CLASS(hsr);
    switch (hsr_ec_class) {
        case HSR_SMC_64_EXCEPTION:
            return handle_smc(vcpu_id, hsr);
        case HSR_WFx_EXCEPTION:
            // If we get a WFI exception, we just do nothing in the VMM.
            return true;
        default:
            LOG_VMM_ERR("unknown SMC exception, EC class: 0x%lx, HSR: 0x%lx\n", hsr_ec_class, hsr);
            return false;
    }
}

bool handle_vppi_event(size_t vcpu_id)
{
    uint64_t ppi_irq = sel4cp_mr_get(seL4_VPPIEvent_IRQ);
    // We directly inject the interrupt assuming it has been previously registered.
    // If not the interrupt will dropped by the VM.
    bool success = vgic_inject_irq(vcpu_id, ppi_irq);
    if (!success) {
        // @ivanv, make a note that when having a lot of printing on it can cause this error
        LOG_VMM_ERR("VPPI IRQ %lu dropped on vCPU %d\n", ppi_irq, vcpu_id);
        // Acknowledge to unmask it as our guest will not use the interrupt
        sel4cp_arm_vcpu_ack_vppi(vcpu_id, ppi_irq);
    }

    return true;
}

bool handle_user_exception(size_t vcpu_id)
{
    // @ivanv: print out VM name/vCPU id when we have multiple VMs
    size_t fault_ip = sel4cp_mr_get(seL4_UserException_FaultIP);
    size_t number = sel4cp_mr_get(seL4_UserException_Number);
    LOG_VMM_ERR("Invalid instruction fault at IP: 0x%lx, number: 0x%lx", fault_ip, number);
    /* All we do is dump the TCB registers. */
    tcb_print_regs(vcpu_id);

    return true;
}

// @ivanv: document where these come from
#define SYSCALL_PA_TO_IPA 65
#define SYSCALL_NOP 67

bool handle_unknown_syscall(size_t vcpu_id)
{
    // @ivanv: should print out the name of the VM the fault came from.
    size_t syscall = sel4cp_mr_get(seL4_UnknownSyscall_Syscall);
    size_t fault_ip = sel4cp_mr_get(seL4_UnknownSyscall_FaultIP);

    LOG_VMM("Received syscall 0x%lx\n", syscall);
    switch (syscall) {
        case SYSCALL_PA_TO_IPA:
            // @ivanv: why do we not do anything here?
            // @ivanv, how to get the physical address to translate?
            LOG_VMM("Received PA translation syscall\n");
            break;
        case SYSCALL_NOP:
            LOG_VMM("Received NOP syscall\n");
            break;
        default:
            LOG_VMM_ERR("Unknown syscall: syscall number: 0x%lx, PC: 0x%lx\n", syscall, fault_ip);
            return false;
    }

    seL4_UserContext regs;
    seL4_Error err = seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + vcpu_id, false, 0, SEL4_USER_CONTEXT_SIZE, &regs);
    assert(err == seL4_NoError);
    if (err != seL4_NoError) {
        LOG_VMM_ERR("Failure reading TCB registers when handling unknown syscall, error %d", err);
        return false;
    }

    return fault_advance_vcpu(vcpu_id, &regs);
}

bool handle_vm_fault(size_t vcpu_id)
{
    uintptr_t addr = sel4cp_mr_get(seL4_VMFault_Addr);
    size_t fsr = sel4cp_mr_get(seL4_VMFault_FSR);

    seL4_UserContext regs;
    int err = seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + vcpu_id, false, 0, SEL4_USER_CONTEXT_SIZE, &regs);
    assert(err == seL4_NoError);

    switch (addr) {
        case GIC_DIST_PADDR...GIC_DIST_PADDR + GIC_DIST_SIZE:
            return handle_vgic_dist_fault(vcpu_id, addr, fsr, &regs);
#if defined(GIC_V3)
        /* Need to handle redistributor faults for GICv3 platforms. */
        case GIC_REDIST_PADDR...GIC_REDIST_PADDR + GIC_REDIST_SIZE:
            return handle_vgic_redist_fault(vcpu_id, addr, fsr, &regs);
#endif
        default: {
            /* If we receive a fault at a location we do not expect, print out as much information
             * as possible for debugging. */
            size_t ip = sel4cp_mr_get(seL4_VMFault_IP);
            size_t is_prefetch = seL4_GetMR(seL4_VMFault_PrefetchFault);
            bool is_write = fault_is_write(fsr);
            LOG_VMM_ERR("unexpected memory fault on address: 0x%lx, FSR: 0x%lx, IP: 0x%lx, is_prefetch: %s, is_write: %s\n",
                addr, fsr, ip, is_prefetch ? "true" : "false", is_write ? "true" : "false");
            tcb_print_regs(vcpu_id);
            vcpu_print_regs(vcpu_id);
            return false;
        }
    }
}
