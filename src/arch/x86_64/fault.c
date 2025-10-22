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

bool fault_handle(size_t vcpu_id, uint64_t *new_rip) {
    bool success = false;

    seL4_Word f_reason = microkit_mr_get(SEL4_VMENTER_FAULT_REASON_MR);
    seL4_Word ins_len = microkit_mr_get(SEL4_VMENTER_FAULT_INSTRUCTION_LEN_MR);
    seL4_Word rip = vmcs_read(vcpu_id, VMX_GUEST_RIP);

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
            // @billn todo revisit likely need to turn on some important features.
            // 3-218 Vol. 2A

            if (vctx.eax == 0) {
                // 3-240 Vol. 2A
                // GenuineIntel
                vctx.eax = 0x1; // ???
                vctx.ebx = 0x756e6547;
                vctx.ecx = 0x49656e69;
                vctx.edx = 0x6c65746e;
            } else if (vctx.eax == 1) {
                // Encoding from:
                // https://en.wikipedia.org/wiki/CPUID
                // "EAX=1: Processor Info and Feature Bits"

                // Values from:
                // https://en.wikichip.org/wiki/intel/cpuid
                // Using value for "Haswell (Client)" Microarch and "HSW-U" Core

                // OEM processor: bit 12 and 13 zero
                uint32_t model_id = 0x5 << 4;
                uint32_t ext_model_id = 0x4 << 16;
                // Pentium and Intel Core family
                uint32_t family_id = 0x6 << 8;

                vctx.eax = model_id | ext_model_id | family_id;

            } else if (vctx.eax == 0x80000000) {
                vctx.eax = 0;
            } else if (vctx.eax == 0x80000001) {
                vctx.eax = 0;
            } else {
                goto ohno;
            }


            vctx.eax = vctx.eax;
            vctx.ebx = vctx.ebx;
            vctx.ecx = vctx.ecx;
            vctx.edx = vctx.edx;
            seL4_X86_VCPU_WriteRegisters(BASE_VCPU_CAP + vcpu_id, &vctx);


            success = true;
            *new_rip = rip + ins_len;
            break;

        case RDMSR:
            if (vctx.ecx == 0xc0000080) {
                uint32_t efer_low = (uint32_t) vmcs_read(GUEST_BOOT_VCPU_ID, VMX_GUEST_EFER);
                vctx.eax = efer_low;
            } else {
                goto ohno;
            }
            success = true;
            *new_rip = rip + ins_len;
            break;
        
        case WRMSR:
            if (vctx.ecx == 0xc0000080) {
                vmcs_write(GUEST_BOOT_VCPU_ID, VMX_GUEST_EFER, vctx.eax);
            } else {
                goto ohno;
            }
            success = true;
            *new_rip = rip + ins_len;
            break;

        default:
ohno:
            LOG_VMM_ERR("unhandled fault\n");
            vcpu_print_regs(vcpu_id);
    };

    return success;
}