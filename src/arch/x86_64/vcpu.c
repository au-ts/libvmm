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

// void vcpu_reset(size_t vcpu_id) {

// }

// bool vcpu_is_on(size_t vcpu_id) {
//     return true;
// }

// void vcpu_set_on(size_t vcpu_id, bool on) {

// }

void vcpu_print_regs(size_t vcpu_id) {
    seL4_Word cppc = microkit_mr_get(SEL4_VMENTER_CALL_CONTROL_PPC_MR);
    seL4_Word vmec = microkit_mr_get(SEL4_VMENTER_CALL_CONTROL_ENTRY_MR);
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
    LOG_VMM("    vm exec control = 0x%lx\n", cppc);
    LOG_VMM("    vm entry control = 0x%lx\n", vmec);
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
    // LOG_VMM("faulting instruction: 0x");
    // for (int i = 0; i < ins_len; i++) {
    //     printf("%hhx", *((char *)(0x80000000 + rip + i)));
    // }
    // printf("\n");
}

