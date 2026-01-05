/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <stdbool.h>

#include <libvmm/vcpu.h>
#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/fault.h>
#include <sel4/arch/vmenter.h>
#include <sddf/util/custom_libc/string.h>

// void vcpu_reset(size_t vcpu_id) {

// }

// bool vcpu_is_on(size_t vcpu_id) {
//     return true;
// }

// void vcpu_set_on(size_t vcpu_id, bool on) {

// }

// Caller must vmenter with EIP 0xFFF0 
void vcpu_set_up_reset_state(void) {
    // prevent the guest from turning of VMX mode
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_CR4_MASK, 1 << 13);

    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_PRIMARY_PROCESSOR_CONTROLS, VMCS_PCC_DEFAULT);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_SECONDARY_PROCESSOR_CONTROLS, VMCS_SPC_DEFAULT | BIT(7)); // unrestricted guest

    // Table 10-1. IA-32 and Intel® 64 Processor States Following Power-up, Reset, or INIT

    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_RFLAGS, 0x2);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_RIP, 0xfff0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CR0, 0x60000010);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CR3, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CR4, 0);

    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CS_SELECTOR, 0xf000);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CS_BASE, 0xffff0000);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CS_LIMIT, 0xffff);
    // Table 25-2. Format of Access Rights
    // 27.3.1.2 Checks on Guest Segment Registers
    //                                                                          type | S | P
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CS_ACCESS_RIGHTS, 3 | BIT(4) | BIT(7));

    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_SS_SELECTOR, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_SS_BASE, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_SS_LIMIT, 0xffff);

    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_SS_ACCESS_RIGHTS, 3 | BIT(4) | BIT(7));

    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_DS_SELECTOR, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_DS_BASE, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_DS_LIMIT, 0xffff);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_DS_ACCESS_RIGHTS, 1 | BIT(4) | BIT(7));

    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_ES_SELECTOR, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_ES_BASE, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_ES_LIMIT, 0xffff);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_ES_ACCESS_RIGHTS, 1 | BIT(4) | BIT(7));

    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_FS_SELECTOR, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_FS_BASE, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_FS_LIMIT, 0xffff);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_FS_ACCESS_RIGHTS, 1 | BIT(4) | BIT(7));

    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_GS_SELECTOR, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_GS_BASE, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_GS_LIMIT, 0xffff);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_GS_ACCESS_RIGHTS, 1 | BIT(4) | BIT(7));

    seL4_VCPUContext vctx;
    memset(&vctx, 0, sizeof(seL4_VCPUContext));
    vctx.edx = 0x00050600;
    microkit_vcpu_x86_write_regs(GUEST_BOOT_VCPU_ID, &vctx);

    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_GDTR_BASE, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_GDTR_LIMIT, 0xffff);

    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_IDTR_BASE, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_IDTR_LIMIT, 0xffff);

    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_LDTR_SELECTOR, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_LDTR_BASE, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_LDTR_LIMIT, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_LDTR_ACCESS_RIGHTS, 2 | BIT(7));

    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_TR_SELECTOR, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_TR_BASE, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_TR_LIMIT, 0);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_TR_ACCESS_RIGHTS, 3 | BIT(7));

    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_EFER, 0);
}

void vcpu_set_up_long_mode(uint64_t cr3, uint64_t gdt_gpa, uint64_t gdt_limit) {
    // @billn explain

    // Set up system registers
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CR0, CR0_DEFAULT);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CR3, cr3);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CR4, CR4_DEFAULT);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_EFER, IA32_EFER_DEFAULT);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_RFLAGS, RFLAGS_DEFAULT);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_GDTR_BASE, gdt_gpa);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_GDTR_LIMIT, gdt_limit);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_PRIMARY_PROCESSOR_CONTROLS, VMCS_PCC_DEFAULT);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_SECONDARY_PROCESSOR_CONTROLS, VMCS_SPC_DEFAULT);

    // @billn explain
    // @billn todo add other important bits

    // prevent the guest from turning of VMX mode
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_CONTROL_CR4_MASK, 1 << 13);
    // @billn todo add other registers with mask

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

    // 25-4 Vol. 3C @billn explain
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_CS_ACCESS_RIGHTS, 0xA09B);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_DS_ACCESS_RIGHTS, 0xC093);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_DS_ACCESS_RIGHTS, 0xC093);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_ES_ACCESS_RIGHTS, 0xC093);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_SS_ACCESS_RIGHTS, 0xC093);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_FS_ACCESS_RIGHTS, 0x3 | 1 << 4 | 1 << 7 | 1 << 15);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_GS_ACCESS_RIGHTS, 0x3 | 1 << 4 | 1 << 7 | 1 << 15);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_LDTR_ACCESS_RIGHTS, 0x2 | 1 << 7);
    microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_TR_ACCESS_RIGHTS, 0xb | 1 << 7);
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

    LOG_VMM("dumping VCPU (ID 0x%lx) registers:\n", vcpu_id);
    LOG_VMM("    vm primary processor control = 0x%lx\n", cppc);
    LOG_VMM("    vm enter irq = 0x%lx\n", irq_info);
    LOG_VMM("    fault reason = 0x%lx (%s)\n", f_reason, fault_to_string(f_reason));
    LOG_VMM("    fault qualification = 0x%lx\n", f_qual);
    LOG_VMM("    instruction length = 0x%lx\n", ins_len);
    LOG_VMM("    guest physical addr = 0x%lx\n", g_p_addr);
    LOG_VMM("    rflags = 0x%lx\n", rflags);
    LOG_VMM("    interruptability = 0x%lx\n", interruptability);
    LOG_VMM("    cr3 = 0x%lx\n", cr3);
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
