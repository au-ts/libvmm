/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <stdbool.h>
#include <string.h>

#include <libvmm/vcpu.h>
#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/util.h>
#include <sel4/arch/vmenter.h>

struct vcpu_fault_state {
    bool valid;
    bool exit_from_ntfn;

    uint64_t original_rip;
    uint64_t resume_rip;
    uint64_t ppvc;
    uint64_t irq;

    uint64_t reason;
    uint64_t qualification;
    uint64_t instruction_length;
    uint64_t gpa;
    uint64_t rflags;
    uint64_t interruptability;
    uint64_t cr3;

    seL4_VCPUContext vctx;
};

static struct vcpu_fault_state vcpu_fault_state;

/* Document referenced:
 * [1] Title: Intel® 64 and IA-32 Architectures Software Developer’s Manual Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *     Order Number: 325462-080US June 2023
 */

bool vcpu_set_up_long_mode(uint64_t cr3, uint64_t gdt_gpa, uint64_t gdt_limit)
{
    /* Set up control registers */
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CR0, CR0_DEFAULT);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CR3, cr3);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CR4, CR4_DEFAULT);
    /* Prevent guest from turning off VM mode */
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_CR4_MASK, CR4_EN_MASK);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_EFER, IA32_EFER_LM_DEFAULT);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_RFLAGS, RFLAGS_DEFAULT);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_GDTR_BASE, gdt_gpa);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_GDTR_LIMIT, gdt_limit);

    /* Check that all CR0 and CR4 features we need are supported by the host.
     * We perform this check because seL4 will clear any unsupported feature bits. */
    uint64_t read_back_cr0 = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CR0);
    if (!check_baseline_bits(CR0_DEFAULT, read_back_cr0)) {
        LOG_VMM_ERR("required Control Register 0 (CR0) features not supported.\n");
        LOG_VMM_ERR("Baseline: 0x%lx, bits sticked: 0x%lx\n", CR0_DEFAULT, read_back_cr0);
        print_missing_baseline_bits(CR0_DEFAULT, read_back_cr0);
        return false;
    }

    uint64_t read_back_cr4 = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CR4);
    if (!check_baseline_bits(CR4_DEFAULT, read_back_cr4)) {
        LOG_VMM_ERR("required Control Register 4 (CR4) features not supported.\n");
        LOG_VMM_ERR("Baseline: 0x%lx, bits sticked: 0x%lx\n", CR4_DEFAULT, read_back_cr4);
        print_missing_baseline_bits(CR4_DEFAULT, read_back_cr4);
        return false;
    }

    /* Set up VMCS control registers */
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_PRIMARY_PROCESSOR_CONTROLS, VMCS_PPVC_DEFAULT);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_SECONDARY_PROCESSOR_CONTROLS, VMCS_SPC_DEFAULT);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_ENTRY_CONTROLS, VMCS_VEC_LM_DEFAULT);

    /* Check that all the VT-x features we need are supported by the host. */
    uint64_t read_back_ppc = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_PRIMARY_PROCESSOR_CONTROLS);
    if (!check_baseline_bits(VMCS_PPVC_DEFAULT, read_back_ppc)) {
        LOG_VMM_ERR("required Primary Processor-Based VM-Execution Controls features not supported.\n");
        LOG_VMM_ERR("Baseline: 0x%lx, bits sticked: 0x%lx\n", VMCS_PPVC_DEFAULT, read_back_ppc);
        print_missing_baseline_bits(VMCS_PPVC_DEFAULT, read_back_ppc);
        return false;
    }

    uint64_t read_back_spc = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_SECONDARY_PROCESSOR_CONTROLS);
    if (!check_baseline_bits(VMCS_SPC_DEFAULT, read_back_spc)) {
        LOG_VMM_ERR("required Secondary Processor-Based VM-Execution Controls features not supported.\n");
        LOG_VMM_ERR("Baseline: 0x%lx, bits sticked: 0x%lx\n", VMCS_SPC_DEFAULT, read_back_spc);
        print_missing_baseline_bits(VMCS_SPC_DEFAULT, read_back_spc);
        return false;
    }

    uint64_t read_back_vec = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_ENTRY_CONTROLS);
    if (!check_baseline_bits(VMCS_VEC_LM_DEFAULT, read_back_vec)) {
        LOG_VMM_ERR("required VM-Entry Controls features not supported.\n");
        LOG_VMM_ERR("Baseline: 0x%lx, bits sticked: 0x%lx\n", VMCS_VEC_LM_DEFAULT, read_back_vec);
        print_missing_baseline_bits(VMCS_VEC_LM_DEFAULT, read_back_vec);
        return false;
    }

    /* These aren't useful in long mode so we just leave them in a sane default enough to boot a guest kernel. */
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CS_BASE, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_DS_BASE, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_ES_BASE, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_SS_BASE, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_FS_BASE, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_GS_BASE, 0);

    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CS_SELECTOR, 8);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_DS_SELECTOR, 16);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_ES_SELECTOR, 16);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_SS_SELECTOR, 16);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_FS_SELECTOR, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_GS_SELECTOR, 0);

    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CS_LIMIT, 0xfffff);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_DS_LIMIT, 0xfffff);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_ES_LIMIT, 0xfffff);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_SS_LIMIT, 0xfffff);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_FS_LIMIT, 0xfffff);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_GS_LIMIT, 0xfffff);

    /* See [1] "Table 25-2. Format of Access Rights" */
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CS_ACCESS_RIGHTS,
                                 0xA09B); // present ring-0 executable 64-bit code segment
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_DS_ACCESS_RIGHTS, 0xC093); // present ring-0 data segment
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_ES_ACCESS_RIGHTS, 0xC093);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_SS_ACCESS_RIGHTS, 0xC093);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_FS_ACCESS_RIGHTS, BIT(16)); // not present
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_GS_ACCESS_RIGHTS, BIT(16)); // not present
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_LDTR_ACCESS_RIGHTS, BIT(16)); // not present
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_TR_ACCESS_RIGHTS, 0xb | 1 << 7); // busy 32-bit TSS

    return true;
}

void vcpu_init_exit_state(bool exit_from_ntfn)
{
    /* Prevent state overwriting, if this goes off then you probably called this or didn't call
     * `vcpu_exit_resume()` at the correct time. */
    assert(!vcpu_fault_state.valid);

    vcpu_fault_state.original_rip = microkit_mr_get(SEL4_VMENTER_CALL_EIP_MR);
    vcpu_fault_state.resume_rip = vcpu_fault_state.original_rip;
    vcpu_fault_state.ppvc = microkit_mr_get(SEL4_VMENTER_CALL_CONTROL_PPC_MR);
    vcpu_fault_state.irq = microkit_mr_get(SEL4_VMENTER_CALL_INTERRUPT_INFO_MR);

    if (!exit_from_ntfn) {
        /* These message registers are only valid if a VM Exit was caused by a fault rather
         * than a notification. */
        vcpu_fault_state.reason = microkit_mr_get(SEL4_VMENTER_FAULT_REASON_MR);
        vcpu_fault_state.qualification = microkit_mr_get(SEL4_VMENTER_FAULT_QUALIFICATION_MR);
        vcpu_fault_state.instruction_length = microkit_mr_get(SEL4_VMENTER_FAULT_INSTRUCTION_LEN_MR);
        vcpu_fault_state.gpa = microkit_mr_get(SEL4_VMENTER_FAULT_GUEST_PHYSICAL_MR);
        vcpu_fault_state.rflags = microkit_mr_get(SEL4_VMENTER_FAULT_RFLAGS_MR);
        vcpu_fault_state.interruptability = microkit_mr_get(SEL4_VMENTER_FAULT_GUEST_INT_MR);
        vcpu_fault_state.cr3 = microkit_mr_get(SEL4_VMENTER_FAULT_CR3_MR);

        vcpu_fault_state.vctx.eax = microkit_mr_get(SEL4_VMENTER_FAULT_EAX);
        vcpu_fault_state.vctx.ebx = microkit_mr_get(SEL4_VMENTER_FAULT_EBX);
        vcpu_fault_state.vctx.ecx = microkit_mr_get(SEL4_VMENTER_FAULT_ECX);
        vcpu_fault_state.vctx.edx = microkit_mr_get(SEL4_VMENTER_FAULT_EDX);
        vcpu_fault_state.vctx.esi = microkit_mr_get(SEL4_VMENTER_FAULT_ESI);
        vcpu_fault_state.vctx.edi = microkit_mr_get(SEL4_VMENTER_FAULT_EDI);
        vcpu_fault_state.vctx.ebp = microkit_mr_get(SEL4_VMENTER_FAULT_EBP);
        vcpu_fault_state.vctx.r8 = microkit_mr_get(SEL4_VMENTER_FAULT_R8);
        vcpu_fault_state.vctx.r9 = microkit_mr_get(SEL4_VMENTER_FAULT_R9);
        vcpu_fault_state.vctx.r10 = microkit_mr_get(SEL4_VMENTER_FAULT_R10);
        vcpu_fault_state.vctx.r11 = microkit_mr_get(SEL4_VMENTER_FAULT_R11);
        vcpu_fault_state.vctx.r12 = microkit_mr_get(SEL4_VMENTER_FAULT_R12);
        vcpu_fault_state.vctx.r13 = microkit_mr_get(SEL4_VMENTER_FAULT_R13);
        vcpu_fault_state.vctx.r14 = microkit_mr_get(SEL4_VMENTER_FAULT_R14);
        vcpu_fault_state.vctx.r15 = microkit_mr_get(SEL4_VMENTER_FAULT_R15);
    }

    vcpu_fault_state.exit_from_ntfn = exit_from_ntfn;
    vcpu_fault_state.valid = true;
}

uint64_t vcpu_exit_get_rip(void)
{
    assert(vcpu_fault_state.valid);
    return vcpu_fault_state.resume_rip;
}

void vcpu_exit_update_ppvc(uint64_t ppvc)
{
    assert(vcpu_fault_state.valid);
    vcpu_fault_state.ppvc = ppvc;
}

uint64_t vcpu_exit_get_irq(void)
{
    assert(vcpu_fault_state.valid);
    return vcpu_fault_state.irq;
}

void vcpu_exit_inject_irq(uint64_t irq)
{
    assert(vcpu_fault_state.valid);
    vcpu_fault_state.irq = irq;
}

uint64_t vcpu_exit_get_reason(void)
{
    assert(vcpu_fault_state.valid);

    /* Shouldn't be called when the VCPU exit is caused by a notification! */
    assert(!vcpu_fault_state.exit_from_ntfn);

    return vcpu_fault_state.reason;
}

uint64_t vcpu_exit_get_qualification(void)
{
    assert(vcpu_fault_state.valid);
    assert(!vcpu_fault_state.exit_from_ntfn);
    return vcpu_fault_state.qualification;
}

uint64_t vcpu_exit_get_instruction_len(void)
{
    assert(vcpu_fault_state.valid);
    assert(!vcpu_fault_state.exit_from_ntfn);
    return vcpu_fault_state.instruction_length;
}

uint64_t vcpu_exit_get_rflags(void)
{
    assert(vcpu_fault_state.valid);

    if (vcpu_fault_state.exit_from_ntfn) {
        return microkit_vcpu_x86_read_vmcs(0, VMX_GUEST_RFLAGS);
    } else {
        return vcpu_fault_state.rflags;
    }
}

uint64_t vcpu_exit_get_interruptability(void)
{
    assert(vcpu_fault_state.valid);
    return vcpu_fault_state.interruptability;
}

uint64_t vcpu_exit_get_cr3(void)
{
    assert(vcpu_fault_state.valid);
    return vcpu_fault_state.cr3;
}

seL4_VCPUContext *vcpu_exit_get_context(void)
{
    assert(vcpu_fault_state.valid);

    if (vcpu_fault_state.exit_from_ntfn) {
        return NULL;
    } else {
        return &vcpu_fault_state.vctx;
    }
}

void vcpu_exit_advance_rip(unsigned rip_additive)
{
    assert(vcpu_fault_state.valid);
    vcpu_fault_state.resume_rip += rip_additive;
}

void vcpu_exit_resume(void)
{
    assert(vcpu_fault_state.valid);

    if (!vcpu_fault_state.exit_from_ntfn) {
        microkit_vcpu_x86_write_regs(0, &vcpu_fault_state.vctx);
    }

    microkit_mr_set(SEL4_VMENTER_CALL_EIP_MR, vcpu_fault_state.resume_rip);
    microkit_mr_set(SEL4_VMENTER_CALL_CONTROL_PPC_MR, vcpu_fault_state.ppvc);
    microkit_mr_set(SEL4_VMENTER_CALL_INTERRUPT_INFO_MR, vcpu_fault_state.irq);
    microkit_vcpu_x86_deferred_resume();
    vcpu_fault_state.valid = false;
}

void vcpu_print_regs(size_t vcpu_id)
{
    seL4_Word cppc = microkit_mr_get(SEL4_VMENTER_CALL_CONTROL_PPC_MR);
    seL4_Word irq_info = microkit_mr_get(SEL4_VMENTER_CALL_INTERRUPT_INFO_MR);
    seL4_Word f_reason = microkit_mr_get(SEL4_VMENTER_FAULT_REASON_MR);
    seL4_Word f_qual = microkit_mr_get(SEL4_VMENTER_FAULT_QUALIFICATION_MR);
    seL4_Word ins_len = microkit_mr_get(SEL4_VMENTER_FAULT_INSTRUCTION_LEN_MR);
    seL4_Word g_p_addr = microkit_mr_get(SEL4_VMENTER_FAULT_GUEST_PHYSICAL_MR);
    seL4_Word rflags = microkit_mr_get(SEL4_VMENTER_FAULT_RFLAGS_MR);
    seL4_Word interruptability = microkit_mr_get(SEL4_VMENTER_FAULT_GUEST_INT_MR);
    seL4_Word cr3 = microkit_mr_get(SEL4_VMENTER_FAULT_CR3_MR);

    seL4_Word rip = microkit_vcpu_x86_read_vmcs(vcpu_id, VMX_GUEST_RIP);
    seL4_Word rsp = microkit_vcpu_x86_read_vmcs(vcpu_id, VMX_GUEST_RSP);

    seL4_Word eax = microkit_mr_get(SEL4_VMENTER_FAULT_EAX);
    seL4_Word ebx = microkit_mr_get(SEL4_VMENTER_FAULT_EBX);
    seL4_Word ecx = microkit_mr_get(SEL4_VMENTER_FAULT_ECX);
    seL4_Word edx = microkit_mr_get(SEL4_VMENTER_FAULT_EDX);
    seL4_Word esi = microkit_mr_get(SEL4_VMENTER_FAULT_ESI);
    seL4_Word edi = microkit_mr_get(SEL4_VMENTER_FAULT_EDI);
    seL4_Word ebp = microkit_mr_get(SEL4_VMENTER_FAULT_EBP);

    seL4_Word r8 = microkit_mr_get(SEL4_VMENTER_FAULT_R8);
    seL4_Word r9 = microkit_mr_get(SEL4_VMENTER_FAULT_R9);
    seL4_Word r10 = microkit_mr_get(SEL4_VMENTER_FAULT_R10);
    seL4_Word r11 = microkit_mr_get(SEL4_VMENTER_FAULT_R11);
    seL4_Word r12 = microkit_mr_get(SEL4_VMENTER_FAULT_R12);
    seL4_Word r13 = microkit_mr_get(SEL4_VMENTER_FAULT_R13);
    seL4_Word r14 = microkit_mr_get(SEL4_VMENTER_FAULT_R14);
    seL4_Word r15 = microkit_mr_get(SEL4_VMENTER_FAULT_R15);

    seL4_Word efer = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_EFER);

    LOG_VMM("dumping VCPU (ID 0x%lx) registers:\n", vcpu_id);
    LOG_VMM("    vm primary processor control = 0x%lx\n", cppc);
    LOG_VMM("    vm enter irq = 0x%lx\n", irq_info);
    LOG_VMM("    fault reason = 0x%lx (%s)\n", f_reason, fault_to_string(f_reason));
    LOG_VMM("    fault qualification = 0x%lx\n", f_qual);
    LOG_VMM("    instruction length = 0x%lx\n", ins_len);
    LOG_VMM("    guest physical addr = 0x%lx\n", g_p_addr);
    LOG_VMM("    rflags = 0x%lx\n", rflags);
    LOG_VMM("    interruptability = 0x%lx\n", interruptability);
    LOG_VMM("    cr0 = 0x%lx\n", microkit_vcpu_x86_read_vmcs(vcpu_id, VMX_GUEST_CR0));
    LOG_VMM("    cr3 = 0x%lx\n", cr3);
    LOG_VMM("    cr4 = 0x%lx\n", microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CR4));
    LOG_VMM("    efer = 0x%lx\n", efer);
    LOG_VMM("=========================\n");
    LOG_VMM("    rip = 0x%lx\n", rip);
    LOG_VMM("    rsp = 0x%lx\n", rsp);
    LOG_VMM("    rax = 0x%lx\n", eax);
    LOG_VMM("    rbx = 0x%lx\n", ebx);
    LOG_VMM("    rcx = 0x%lx\n", ecx);
    LOG_VMM("    rdx = 0x%lx\n", edx);
    LOG_VMM("    rsi = 0x%lx\n", esi);
    LOG_VMM("    rdi = 0x%lx\n", edi);
    LOG_VMM("    rbp = 0x%lx\n", ebp);
    LOG_VMM("    r8 = 0x%lx\n", r8);
    LOG_VMM("    r9 = 0x%lx\n", r9);
    LOG_VMM("    r10 = 0x%lx\n", r10);
    LOG_VMM("    r11 = 0x%lx\n", r11);
    LOG_VMM("    r12 = 0x%lx\n", r12);
    LOG_VMM("    r13 = 0x%lx\n", r13);
    LOG_VMM("    r14 = 0x%lx\n", r14);
    LOG_VMM("    r15 = 0x%lx\n", r15);
    LOG_VMM("=========================\n");
}
