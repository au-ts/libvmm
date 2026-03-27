/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sddf/util/util.h>
#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/msr.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/util.h>
#include <libvmm/arch/x86_64/memory_space.h>

#include <x86intrin.h>

// Table 2-2. IA-32 Architectural MSRs

#define IA32_TIME_STAMP_COUNTER (0x10)
#define IA32_PLATFORM_ID (0x17)
#define IA32_SPEC_CTRL (0x48)
#define IA32_PRED_CMD (0x49)
#define IA32_PPIN_CTL (0x4e)
#define IA32_BIOS_UPDT_TRIG (0x79)
#define IA32_MKTME_KEYID_PARTITIONING (0x87)
#define IA32_BIOS_SIGN_ID (0x8b)
#define IA32_CORE_CAPABILITIES (0xcf)
#define IA32_MTRRCAP (0xfe)
#define IA32_MISC_ENABLE (0x1a0)
#define IA32_MCG_CAP (0x179)
#define IA32_MCG_STATUS (0x17a)
#define IA32_PAT (0x277)
#define IA32_MTRR_DEF_TYPE (0x2ff)
#define IA32_XSS (0xda0)

#define MSR_RAPL_POWER_UNIT  (0x606)
#define MSR_PKG_ENERGY_STATUS (0x611)
#define MSR_PP0_ENERGY_STATUS (0x639)
#define MSR_DRAM_ENERGY_STATUS (0x619)
#define MSR_PP1_ENERGY_STATUS (0x641)
#define MSR_PLATFORM_ENERGY_COUNTER (0x64d)
#define MSR_PPERF (0x64e)
#define MSR_SMI_COUNT (0x34)
#define MSR_CORE_C3_RESIDENCY (0x3fc)
#define MSR_CORE_C6_RESIDENCY (0x3fd)
#define MSR_CORE_C7_RESIDENCY (0x3fe)
#define MSR_PKG_C2_RESIDENCY (0x60d)
#define MSR_PKG_C2_RESIDENCY_ALT (0x3f8)
#define MSR_PKG_C4_RESIDENCY (0x3f9)
#define MSR_PKG_C6_RESIDENCY (0x3fa)
#define MSR_OS_MAILBOX_INTERFACE		0xB0
#define MSR_OS_MAILBOX_DATA			0xB1

// @billn seems to be some sort of fencing instruction control
// https://gruss.cc/files/msrtemplating.pdf
#define MSR_UNKNOWN1 (0xc0011029)

#define IA32_APIC_BASE (0x1b)
#define IA32_FEATURE_CONTROL (0x3a)
#define MISC_FEATURE_ENABLES (0x140)
#define MSR_PLATFORM_INFO (0xce)

#define MSR_TEST_CTRL (0x33)

/* x86-64 specific MSRs */
#define MSR_EFER            0xc0000080 /* extended feature register */
#define MSR_STAR            0xc0000081 /* legacy mode SYSCALL target */
#define MSR_LSTAR           0xc0000082 /* long mode SYSCALL target */
#define MSR_CSTAR           0xc0000083 /* compat mode SYSCALL target */
#define MSR_SYSCALL_MASK    0xc0000084 /* EFLAGS mask for syscall */

bool emulate_rdmsr(seL4_VCPUContext *vctx)
{
    uint64_t result = 0;

    switch (vctx->ecx) {
    case MSR_EFER: {
        result = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_EFER);
        break;
    }
    case IA32_TIME_STAMP_COUNTER:
        result = __rdtsc();
        break;
    case MSR_STAR:
    case MSR_LSTAR:
    case MSR_CSTAR:
    case MSR_SYSCALL_MASK:
        result = microkit_vcpu_x86_read_msr(GUEST_BOOT_VCPU_ID, vctx->ecx);
        break;
    case IA32_APIC_BASE:
        // Figure 11-5. IA32_APIC_BASE MSR (APIC_BASE_MSR in P6 Family)
        //                   enable    is boot cpu
        result = LAPIC_GPA | BIT(11) | BIT(8);
        break;
    case IA32_MCG_CAP:
    case IA32_MCG_STATUS:
    case IA32_MTRRCAP:
    case IA32_MTRR_DEF_TYPE:
    case IA32_PAT:
        // @billn revisit above 5
    case IA32_SPEC_CTRL:
        // @billn revisit, I think we should use Virtualize IA32_SPEC_CTRL
        // in Tertiary Processor-Based VM-Execution Controls
        break;
    default:
        return false;
    }

    LOG_FAULT("handling RDMSR 0x%x, result 0x%lx\n", vctx->ecx, result);

    vctx->eax = result & 0xffffffff;
    vctx->edx = (result >> 32) & 0xffffffff;
    return true;
}

bool emulate_wrmsr(seL4_VCPUContext *vctx)
{
    uint64_t value = (uint64_t)((vctx->edx & 0xffffffff) << 32) | (uint64_t)(vctx->eax & 0xffffffff);

    LOG_FAULT("handling WRMSR 0x%x, value 0x%lx\n", vctx->ecx, value);

    switch (vctx->ecx) {
    case MSR_EFER:
        microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_EFER, value);
        break;
    case MSR_STAR:
    case MSR_LSTAR:
    case MSR_CSTAR:
    case MSR_SYSCALL_MASK:
        microkit_vcpu_x86_write_msr(GUEST_BOOT_VCPU_ID, vctx->ecx, value);
        return true;
    case IA32_PRED_CMD:
        // @billn revisit, security concerns same as IA32_SPEC_CTRL, as they are used for speculative exec controls
        break;
    default:
        LOG_VMM("unknown wrmsr 0x%x, value 0x%lx\n", vctx->ecx, value);
        return false;
    }

    return true;
}
