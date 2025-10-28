/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string.h>
#include <sddf/util/util.h>
#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/cpuid.h>
#include <libvmm/arch/x86_64/msr.h>
#include <libvmm/arch/x86_64/apic.h>
#include <sel4/arch/vmenter.h>

/* Documents referenced:
 * [1] seL4: include/arch/x86/arch/object/vcpu.h
 */

/* Exit reasons. 
 * From [1]
 */
enum exit_reasons {
    EXCEPTION_OR_NMI = 0x00,
    EXTERNAL_INTERRUPT = 0x01,
    TRIPLE_FAULT = 0x02,
    INIT_SIGNAL = 0x03,
    SIPI = 0x04,
    /*IO_SMI = 0x05,
     *   OTHER_SMI = 0x06,*/
    INTERRUPT_WINDOW = 0x07,
    NMI_WINDOW = 0x08,
    TASK_SWITCH = 0x09,
    CPUID = 0x0A,
    GETSEC = 0x0B,
    HLT = 0x0C,
    INVD = 0x0D,
    INVLPG = 0x0E,
    RDPMC = 0x0F,
    RDTSC = 0x10,
    RSM = 0x11,
    VMCALL = 0x12,
    VMCLEAR = 0x13,
    VMLAUNCH = 0x14,
    VMPTRLD = 0x15,
    VMPTRST = 0x16,
    VMREAD = 0x17,
    VMRESUME = 0x18,
    VMWRITE = 0x19,
    VMXOFF = 0x1A,
    VMXON = 0x1B,
    CONTROL_REGISTER = 0x1C,
    MOV_DR = 0x1D,
    IO = 0x1E,
    RDMSR = 0x1F,
    WRMSR = 0x20,
    INVALID_GUEST_STATE = 0x21,
    MSR_LOAD_FAIL = 0x22,
    /* 0x23 */
    MWAIT = 0x24,
    MONITOR_TRAP_FLAG = 0x25,
    /* 0x26 */
    MONITOR = 0x27,
    PAUSE = 0x28,
    MACHINE_CHECK = 0x29,
    /* 0x2A */
    TPR_BELOW_THRESHOLD = 0x2B,
    APIC_ACCESS = 0x2C,
    GDTR_OR_IDTR = 0x2E,
    LDTR_OR_TR = 0x2F,
    EPT_VIOLATION = 0x30,
    EPT_MISCONFIGURATION = 0x31,
    INVEPT = 0x32,
    RDTSCP = 0x33,
    VMX_PREEMPTION_TIMER = 0x34,
    INVVPID = 0x35,
    WBINVD = 0x36,
    XSETBV = 0x37
};

#define APIC_BASE 0xFFFE0000
#define APIC_SIZE 0x1000

/* Table 29-7. Exit Qualification for EPT Violations */
#define EPT_VIOLATION_READ (1 << 0)
#define EPT_VIOLATION_WRITE (1 << 1)

bool fault_is_read(seL4_Word qualification) {
    return qualification & EPT_VIOLATION_READ;
}

bool fault_is_write(seL4_Word qualification) {
    return qualification & EPT_VIOLATION_WRITE;
}

bool emulate_vmfault(seL4_VCPUContext *vctx, seL4_Word qualification) {
    uint64_t addr = microkit_mr_get(SEL4_VMENTER_FAULT_GUEST_PHYSICAL_MR);
    LOG_VMM("handling fault on 0x%lx, qualification: 0x%lx\n", addr, qualification);

    if (addr >= APIC_BASE && addr < APIC_BASE + APIC_SIZE) {
        return apic_fault_handle(addr - APIC_BASE, qualification);
    }

    return false;
}

#define X86_MAX_INSTRUCTION_LENGTH 15

extern uint64_t guest_ram_vaddr;

/* Convert guest physical address to the VMM's virtual memory. */
void *gpa_to_vaddr(uint64_t gpa) {
    return (void *)(guest_ram_vaddr + gpa);
}

seL4_Word pte_to_gpa(seL4_Word pte) {
    return pte & 0xffffffffff000;
}

uint64_t fault_instruction(size_t vcpu_id, seL4_Word rip, seL4_Word instruction_len) {

    LOG_VMM("getting instruction at GVA 0x%lx\n", rip);

    seL4_Word cr4 = vmcs_read(vcpu_id, VMX_GUEST_CR4);

    LOG_VMM("cr4: 0x%lx\n", cr4);

    seL4_Word cr3 = vmcs_read(vcpu_id, VMX_GUEST_CR3);

    seL4_Word pml4_gpa = (cr3 >> 12) << 12;

    LOG_VMM("pml4_gpa: 0x%lx\n", pml4_gpa);

    seL4_Word *pml4 = gpa_to_vaddr(pml4_gpa);

    seL4_Word pml4_idx = (rip >> (12 + (9 * 3))) & 0x1ff;
    LOG_VMM("pml4_idx: 0x%lx\n", pml4_idx);

    seL4_Word pdpt_gpa = pte_to_gpa(pml4[pml4_idx]);
    LOG_VMM("pdpt_gpa: 0x%lx\n", pdpt_gpa);
    uint64_t *pdpt = gpa_to_vaddr(pdpt_gpa);

    seL4_Word pdpt_idx = (rip >> (12 + (9 * 2))) & 0x1ff;
    seL4_Word pd_gpa = pte_to_gpa(pdpt[pdpt_idx]);
    LOG_VMM("pd_gpa: 0x%lx\n", pd_gpa);
    uint64_t *pd = gpa_to_vaddr(pd_gpa);

    seL4_Word pt_idx = (rip >> (12 + 9)) & 0x1ff;
    seL4_Word pt_gpa = pte_to_gpa(pd[pt_idx]);
    LOG_VMM("pt_gpa: 0x%lx\n", pt_gpa);
    uint64_t *pt = gpa_to_vaddr(pt_gpa);

    seL4_Word page_idx = (rip >> (12)) & 0x1ff;
    seL4_Word page_gpa = pte_to_gpa((pt[page_idx])) + rip & 0x1ff;
    uint64_t *page = gpa_to_vaddr(page_gpa);
    LOG_VMM("page: 0x%lx\n", page);

    assert(instruction_len <= X86_MAX_INSTRUCTION_LENGTH);
    uint8_t instruction_buf[X86_MAX_INSTRUCTION_LENGTH];
    memcpy(instruction_buf, (uint8_t *)page, instruction_len);

    LOG_VMM("decoded instruction:\n");
    for (int i = 0; i < instruction_len; i++) {
        LOG_VMM("[%d]: 0x%x\n", i, instruction_buf[i]);
    }

    return 0;
}

// @billn todo exit reason -> human frenly string

bool fault_handle(size_t vcpu_id, uint64_t *new_rip) {
    bool success = false;

    seL4_Word f_reason = microkit_mr_get(SEL4_VMENTER_FAULT_REASON_MR);
    seL4_Word ins_len = microkit_mr_get(SEL4_VMENTER_FAULT_INSTRUCTION_LEN_MR);
    seL4_Word qualification = microkit_mr_get(SEL4_VMENTER_FAULT_QUALIFICATION_MR);
    seL4_Word rip = microkit_mr_get(SEL4_VMENTER_CALL_EIP_MR);

    seL4_VCPUContext vctx;
    vctx.eax = microkit_mr_get(SEL4_VMENTER_FAULT_EAX);
    vctx.ebx = microkit_mr_get(SEL4_VMENTER_FAULT_EBX);
    vctx.ecx = microkit_mr_get(SEL4_VMENTER_FAULT_ECX);
    vctx.edx = microkit_mr_get(SEL4_VMENTER_FAULT_EDX);
    vctx.esi = microkit_mr_get(SEL4_VMENTER_FAULT_ESI);
    vctx.edi = microkit_mr_get(SEL4_VMENTER_FAULT_EDI);
    vctx.ebp = microkit_mr_get(SEL4_VMENTER_FAULT_EBP);
    vctx.r8 = microkit_mr_get(SEL4_VMENTER_FAULT_R8);
    vctx.r9 = microkit_mr_get(SEL4_VMENTER_FAULT_R9);
    vctx.r10 = microkit_mr_get(SEL4_VMENTER_FAULT_R10);
    vctx.r11 = microkit_mr_get(SEL4_VMENTER_FAULT_R11);
    vctx.r12 = microkit_mr_get(SEL4_VMENTER_FAULT_R12);
    vctx.r13 = microkit_mr_get(SEL4_VMENTER_FAULT_R13);
    vctx.r14 = microkit_mr_get(SEL4_VMENTER_FAULT_R14);
    vctx.r15 = microkit_mr_get(SEL4_VMENTER_FAULT_R15);

    switch (f_reason) {
        case CPUID:
            success = emulate_cpuid(&vctx);
            break;
        case RDMSR:
            success = emulate_rdmsr(&vctx);
            break;
        case WRMSR:
            success = emulate_wrmsr(&vctx);
            break;
        case EPT_VIOLATION:
            fault_instruction(vcpu_id, rip, ins_len);
            success = emulate_vmfault(&vctx, qualification);
            break;
        default:
            LOG_VMM_ERR("unhandled fault: 0x%x\n", f_reason);
            vcpu_print_regs(vcpu_id);
    };

    if (success) {
        seL4_X86_VCPU_WriteRegisters(BASE_VCPU_CAP + vcpu_id, &vctx);
        *new_rip = rip + ins_len;
    } else {
        LOG_VMM_ERR("failed handling fault: 0x%x\n", f_reason);
        vcpu_print_regs(vcpu_id);
    }

    return success;
}