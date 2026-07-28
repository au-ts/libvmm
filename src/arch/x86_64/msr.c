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
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/guest_time.h>
#include <libvmm/arch/x86_64/memory_space.h>
#include <libvmm/arch/x86_64/mtrr.h>

#include <x86intrin.h>

/* Documents referenced:
 * 1. Intel® 64 and IA-32 Architectures Software Developer’s Manual
 *    Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *    Order Number: 325462-080US June 2023
 */

/* [1] "Table 2-2. IA-32 Architectural MSRs" */
#define IA32_TIME_STAMP_COUNTER (0x10)
#define IA32_PLATFORM_ID (0x17)
#define IA32_SPEC_CTRL (0x48)
#define IA32_PRED_CMD (0x49)
#define IA32_BIOS_SIGN_ID (0x8b)
#define IA32_MISC_ENABLE (0x1a0)
#define IA32_MCG_CAP (0x179)
#define IA32_MCG_STATUS (0x17a)
#define IA32_PAT (0x277)
#define IA32_XSS (0xda0)

#define IA32_APIC_BASE (0x1b)
#define IA32_FEATURE_CONTROL (0x3a)

/* x86-64 specific MSRs */
#define MSR_EFER            0xc0000080 /* extended feature register */
#define MSR_STAR            0xc0000081 /* legacy mode SYSCALL target */
#define MSR_LSTAR           0xc0000082 /* long mode SYSCALL target */
#define MSR_CSTAR           0xc0000083 /* compat mode SYSCALL target */
#define MSR_SYSCALL_MASK    0xc0000084 /* EFLAGS mask for syscall */

static bool msrs_initialised = false;
static uint32_t apic_base_msr_mask = 0;

bool initialise_msrs(bool bsp)
{
    if (bsp) {
        /* Figure 11-5. IA32_APIC_BASE MSR (APIC_BASE_MSR in P6 Family)
         * Is a boot strap processor. */
        apic_base_msr_mask = BIT(8);

        assert(initialise_mtrr());
    }

    msrs_initialised = true;

    return true;
}

bool emulate_rdmsr(seL4_VCPUContext *vctx)
{
    if (!msrs_initialised) {
        LOG_VMM_ERR("MSRs not initialised!\n");
        return false;
    }

    uint64_t result = 0;

    if (msr_is_mtrr(vctx->ecx, true)) {
        assert(handle_mtrr_msr_read(vctx->ecx, &result));
    } else {
        switch (vctx->ecx) {
        case MSR_EFER:
            result = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_EFER);
            break;
        case IA32_TIME_STAMP_COUNTER:
            result = guest_time_tsc_now();
            break;
        case MSR_STAR:
        case MSR_LSTAR:
        case MSR_CSTAR:
        case MSR_SYSCALL_MASK:
            result = microkit_vcpu_x86_read_msr(GUEST_BOOT_VCPU_ID, vctx->ecx);
            break;
        case IA32_APIC_BASE:
            /* Figure 11-5. IA32_APIC_BASE MSR (APIC_BASE_MSR in P6 Family)
             *                   enable    is boot cpu? */
            result = LAPIC_GPA | BIT(11) | apic_base_msr_mask;
            break;
        case IA32_SPEC_CTRL:
            // @billn revisit, I think we should use Virtualize IA32_SPEC_CTRL
            // in Tertiary Processor-Based VM-Execution Controls
            break;
        case IA32_PAT:
            result = microkit_vcpu_x86_read_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_PAT);
            break;
        case IA32_PLATFORM_ID:
            result = 0;
            break;
        default:
            LOG_FAULT("unknown MSR read 0x%lx\n", vctx->ecx);
            return false;
        }
    }

    LOG_FAULT("handling RDMSR 0x%lx, result 0x%lx\n", vctx->ecx, result);

    vctx->eax = result & 0xffffffff;
    vctx->edx = (result >> 32) & 0xffffffff;
    return true;
}

bool emulate_wrmsr(seL4_VCPUContext *vctx)
{
    if (!msrs_initialised) {
        LOG_VMM_ERR("MSRs not initialised!\n");
        return false;
    }

    uint64_t value = (uint64_t)((vctx->edx & 0xffffffff) << 32) | (uint64_t)(vctx->eax & 0xffffffff);

    LOG_FAULT("handling WRMSR 0x%lx, value 0x%lx\n", vctx->ecx, value);

    if (msr_is_mtrr(vctx->ecx, false)) {
        assert(handle_mtrr_msr_write(vctx->ecx, value));
    } else {
        switch (vctx->ecx) {
        case IA32_APIC_BASE:
        /* Make sure that the guest isn't transitioning our virtual APIC into an invalid state.
         * See Figure 11-5. IA32_APIC_BASE MSR (APIC_BASE_MSR in P6 Family) for bit definitions. */
            if (value & BIT(10)) {
                LOG_VMM_ERR("guest tried to enable x2APIC via IA32_APIC_BASE\n");
                return false;
            }

            uint64_t requested_apic_base = value & ~0xFFFull;
            if (requested_apic_base != LAPIC_GPA) {
                LOG_VMM_ERR("Guest tried to relocate APIC base to 0x%lx\n", requested_apic_base);
                return false;
            }

            if (!(value & BIT(11))) {
                LOG_VMM_ERR("Guest tried to globally disable the APIC (not supported)\n");
                return false;
            }

            break;
        case MSR_EFER:
            microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_EFER, value);
            break;
        case IA32_PAT:
            microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_PAT, value);
            break;
        case MSR_STAR:
        case MSR_LSTAR:
        case MSR_CSTAR:
        case MSR_SYSCALL_MASK:
            microkit_vcpu_x86_write_msr(GUEST_BOOT_VCPU_ID, vctx->ecx, value);
            return true;
        case IA32_PRED_CMD:
        case IA32_SPEC_CTRL:
            // @billn revisit, security concerns same as IA32_SPEC_CTRL, as they are used for speculative exec controls
            break;
        case IA32_XSS:
            if (value != 0) {
                LOG_VMM_ERR("unexpected value 0x%lx written to IA32_XSS\n", value);
            }
            break;
        default:
            LOG_VMM("unknown wrmsr 0x%lx, value 0x%lx\n", vctx->ecx, value);
            return false;
        }
    }

    return true;
}
