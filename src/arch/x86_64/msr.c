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

#define IA32_PLATFORM_ID (0x17)
#define IA32_BIOS_SIGN_ID (0x8b)
#define IA32_CORE_CAPABILITIES (0xcf)
#define IA32_MISC_ENABLE (0x1a0)

#define MSR_TEST_CTRL (0x33)

/* x86-64 specific MSRs */
#define MSR_EFER            0xc0000080 /* extended feature register */
#define MSR_STAR            0xc0000081 /* legacy mode SYSCALL target */
#define MSR_LSTAR           0xc0000082 /* long mode SYSCALL target */
// #define MSR_CSTAR           0xc0000083 /* compat mode SYSCALL target */
#define MSR_SYSCALL_MASK    0xc0000084 /* EFLAGS mask for syscall */
// #define MSR_FS_BASE         0xc0000100 /* 64bit FS base */
// #define MSR_GS_BASE         0xc0000101 /* 64bit GS base */
// #define MSR_SHADOW_GS_BASE  0xc0000102 /* SwapGS GS shadow */
// #define MSR_TSC_AUX         0xc0000103 /* Auxiliary TSC */

bool emulate_rdmsr(seL4_VCPUContext *vctx) {
    switch (vctx->ecx) {
    case MSR_EFER: {
        uint32_t efer_low = (uint32_t) microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_EFER);
        vctx->eax = efer_low;
        vctx->edx = 0;
        break;
    }
    case IA32_PLATFORM_ID:
    case IA32_CORE_CAPABILITIES:
    case IA32_MISC_ENABLE:
    case IA32_BIOS_SIGN_ID:
    case MSR_TEST_CTRL:
        vctx->eax = 0;
        vctx->edx = 0;
        break;
    default:
        LOG_VMM_ERR("unknown rdmsr 0x%x\n", vctx->ecx);
        return false;
    }

    return true;
}

bool emulate_wrmsr(seL4_VCPUContext *vctx) {

    switch (vctx->ecx) {
    case MSR_EFER:
        microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_EFER, vctx->eax);
        break;
    case MSR_TEST_CTRL:
    case IA32_BIOS_SIGN_ID:
        return true;
    case MSR_STAR:
    case MSR_LSTAR:
    case MSR_SYSCALL_MASK:
        microkit_vcpu_x86_write_msr(GUEST_BOOT_VCPU_ID, vctx->ecx, (vctx->edx << 32) | (vctx->eax));
        return true;
    default:
        LOG_VMM("unknown wrmsr 0x%x\n", vctx->ecx);
        return false;
    }

    return true;
}

