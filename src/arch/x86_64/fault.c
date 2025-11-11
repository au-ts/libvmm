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
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/cpuid.h>
#include <libvmm/arch/x86_64/msr.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <sel4/arch/vmenter.h>

/* Documents referenced:
 * [1] seL4: include/arch/x86/arch/object/vcpu.h
 * [2] Title: Intel® 64 and IA-32 Architectures Software Developer’s Manual Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4 Order Number: 325462-080US June 2023
 *   [2a] Location: Table C-1 "VMX BASIC EXIT REASONS", page: "Vol. 3D C-1"
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
    XSETBV = 0x37,
    NUM_EXIT_REASONS = 0x38,
};

/* [2a] */
static char *exit_reason_strs[NUM_EXIT_REASONS] = {
    "Exception or non-maskable interrupt (NMI)",
    "External interrupt",
    "Triple Fault",
    "INIT signal",
    "Start-up IPI (SIPI)",
    "I/O system-management interrupt (SMI)",
    "Other SMI",
    "Interrupt window",
    "NMI window",
    "Task switch",
    "CPUID",
    "GETSEC",
    "HLT",
    "INVD",
    "INVLPG",
    "RDPMC",
    "RDTSC",
    "RSM",
    "VMCALL",
    "VMCLEAR",
    "VMLAUNCH",
    "VMPTRLD",
    "VMPTRST",
    "VMREAD",
    "VMRESUME",
    "VMWRITE",
    "VMXOFF",
    "VMXON",
    "Control-register accesses",
    "MOV DR",
    "I/O instruction",
    "RDMSR",
    "WRMSR",
    "VM-entry failure due to invalid guest state",
    "VM-entry failure due to MSR loading",
    "",
    "MWAIT",
    "Monitor trap flag",
    "",
    "MONITOR",
    "PAUSE",
    "VM-entry failure due to machine-check event",
    "",
    "TPR below threshold",
    "APIC access",
    "Virtualized EOI",
    "Access to GDTR or IDTR",
    "Access to LDTR or TR",
    "EPT violation",
    "EPT misconfiguration",
    "INVEPT",
    "RDTSCP",
    "VMX-preemption timer expired",
    "INVVPID",
    "WBINVD or WBNOINVD",
    "XSETBV",
};

char *fault_to_string(int exit_reason) {
    assert(exit_reason < NUM_EXIT_REASONS);
    return exit_reason_strs[exit_reason];
}

// @billn dedup
#define LAPIC_BASE 0xFFFE0000
#define LAPIC_SIZE 0x1000

#define IOAPIC_BASE 0x11000000
#define IOAPIC_SIZE 0x1000

/* Table 29-7. Exit Qualification for EPT Violations */
#define EPT_VIOLATION_READ (1 << 0)
#define EPT_VIOLATION_WRITE (1 << 1)

bool ept_fault_is_read(seL4_Word qualification) {
    return qualification & EPT_VIOLATION_READ;
}

bool ept_fault_is_write(seL4_Word qualification) {
    return qualification & EPT_VIOLATION_WRITE;
}

bool emulate_vmfault(seL4_VCPUContext *vctx, seL4_Word qualification, memory_instruction_data_t decoded_mem_ins) {
    uint64_t addr = microkit_mr_get(SEL4_VMENTER_FAULT_GUEST_PHYSICAL_MR);
    // LOG_VMM("handling EPT fault on GPA 0x%lx, qualification: 0x%lx\n", addr, qualification);

    if (addr >= LAPIC_BASE && addr < LAPIC_BASE + LAPIC_SIZE) {
        return lapic_fault_handle(vctx, addr - LAPIC_BASE, qualification, decoded_mem_ins);
    } else if (addr >= IOAPIC_BASE && addr < IOAPIC_BASE + IOAPIC_SIZE) {
        return ioapic_fault_handle(vctx, addr - IOAPIC_BASE, qualification, decoded_mem_ins);
    }

    return false;
}

bool fault_handle(size_t vcpu_id, uint64_t *new_rip) {
    bool success = false;
    decoded_instruction_ret_t decoded_ins;

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
            decoded_ins = decode_instruction(vcpu_id, rip, ins_len);
            assert(decoded_ins.type == INSTRUCTION_MEMORY);
            success = emulate_vmfault(&vctx, qualification, decoded_ins.decoded.memory_instruction);
            break;
        case IO:
            success = emulate_ioports(&vctx, qualification);
            break;
        default:
            LOG_VMM_ERR("unhandled fault: 0x%x\n", f_reason);
    };

    if (success) {
        microkit_vcpu_x86_write_regs(vcpu_id, &vctx);
        *new_rip = rip + ins_len;
    } else {
        LOG_VMM_ERR("failed handling fault: 0x%x\n", f_reason);
        vcpu_print_regs(vcpu_id);
    }

    return success;
}