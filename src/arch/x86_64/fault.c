/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sddf/util/util.h>
#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/vcpu.h>
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

// @billn todo exit reason -> human frenly string

// bool fault_is_read(seL4_Word fsr) {
//     assert(false);
//     return false;
// }

// bool fault_is_write(seL4_Word fsr) {
//     assert(false);
//     return false;
// }

// char *fault_to_string(seL4_Word fault_label)
// {
//     switch (fault_label) {
//     case seL4_Fault_VMFault:
//         return "virtual memory exception";
//     case seL4_Fault_UserException:
//         return "user exception";
//     default:
//         return "unknown fault";
//     }
// }

// void dump_user_exception(void) {
//     LOG_VMM("user exception fault with:\n");
//     LOG_VMM("  PC: 0x%lx\n", microkit_mr_get(seL4_UserException_FaultIP));
//     LOG_VMM("  SP: 0x%lx\n", microkit_mr_get(seL4_UserException_SP));
//     LOG_VMM("  FLAGS: 0x%lx\n", microkit_mr_get(seL4_UserException_FLAGS));
//     LOG_VMM("  Number: 0x%lx\n", microkit_mr_get(seL4_UserException_Number));
//     LOG_VMM("  Code: 0x%lx\n", microkit_mr_get(seL4_UserException_Code));
// }

bool fault_handle(size_t vcpu_id) {
    bool success = false;

    seL4_Word f_reason = microkit_mr_get(SEL4_VMENTER_FAULT_REASON_MR);
    switch (f_reason) {
        case CPUID:

            break;
        default:
            LOG_VMM_ERR("unhandled fault\n");
            vcpu_print_regs(vcpu_id);
    };

    return success;
}