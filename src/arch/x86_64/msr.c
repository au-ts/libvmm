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
#define IA32_PPIN_CTL (0x4e)
#define IA32_BIOS_UPDT_TRIG (0x79)
#define IA32_MKTME_KEYID_PARTITIONING (0x87)
#define IA32_BIOS_SIGN_ID (0x8b)
#define IA32_CORE_CAPABILITIES (0xcf)
#define IA32_MISC_ENABLE (0x1a0)
#define IA32_MCG_CAP (0x179)
#define IA32_MCG_STATUS (0x17a)
#define IA32_PAT (0x277)
#define IA32_XSS (0xda0)

#define IA32_MTRRCAP (0xfe)
#define IA32_MTRR_DEF_TYPE (0x2ff)
#define IA32_MTRR_PHYSBASE0 (0x200)
#define IA32_MTRR_PHYSMASK0 (0x201)
#define IA32_MTRR_PHYSBASE1 (0x202)
#define IA32_MTRR_PHYSMASK1 (0x203)
#define IA32_MTRR_PHYSBASE2 (0x204)
#define IA32_MTRR_PHYSMASK2 (0x205)
#define IA32_MTRR_PHYSBASE3 (0x206)
#define IA32_MTRR_PHYSMASK3 (0x207)
#define IA32_MTRR_PHYSBASE4 (0x208)
#define IA32_MTRR_PHYSMASK4 (0x209)
#define IA32_MTRR_PHYSBASE5 (0x20A)
#define IA32_MTRR_PHYSMASK5 (0x20B)
#define IA32_MTRR_PHYSBASE6 (0x20C)
#define IA32_MTRR_PHYSMASK6 (0x20D)
#define IA32_MTRR_PHYSBASE7 (0x20E)
#define IA32_MTRR_PHYSMASK7 (0x20F)
#define IA32_MTRR_FIX64K_00000 (0x250)
#define IA32_MTRR_FIX16K_80000 (0x258)
#define IA32_MTRR_FIX16K_A0000 (0x259)
#define IA32_MTRR_FIX4K_C0000 (0x268)
#define IA32_MTRR_FIX4K_C8000 (0x269)
#define IA32_MTRR_FIX4K_D0000 (0x26A)
#define IA32_MTRR_FIX4K_D8000 (0x26B)
#define IA32_MTRR_FIX4K_E0000 (0x26C)
#define IA32_MTRR_FIX4K_E8000 (0x26D)
#define IA32_MTRR_FIX4K_F0000 (0x26E)
#define IA32_MTRR_FIX4K_F8000 (0x26F)

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

/* MTRRs virtualisation */
#define MTRR_MAX_VARIABLES 8

struct mtrr_state {
    uint64_t mtrr_def_type;
    uint64_t fixed_64k;    /* 0x250 */
    uint64_t fixed_16k[2]; /* 0x258, 0x259 */
    uint64_t fixed_4k[8];  /* 0x268 - 0x26F */

    struct {
        uint64_t base;
        uint64_t mask;
    } variable[MTRR_MAX_VARIABLES];
};

static bool msrs_initialised = false;
static uint32_t apic_base_msr_mask = 0;
static struct mtrr_state mtrr_state;

bool initialise_msrs(bool bsp)
{
    if (bsp) {
        /* Figure 11-5. IA32_APIC_BASE MSR (APIC_BASE_MSR in P6 Family)
         * Is a boot strap processor. */
        apic_base_msr_mask = BIT(8);

        /* See "IA32_MTRR_DEF_TYPE MSR"
         * Enable MTRRs */
        mtrr_state.mtrr_def_type = BIT(11);
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
    case IA32_MTRRCAP:
        /* "Table 2-2. IA-32 Architectural MSRs (Contd.)" "IA32_MTRRCAP (MTRRcap)"
           "Fixed range MTRRs are supported when set" + WC supported. */
        result = MTRR_MAX_VARIABLES | BIT(8) | BIT(10);
        break;
    case IA32_MTRR_DEF_TYPE:
        result = mtrr_state.mtrr_def_type;
        break;
    case IA32_MTRR_PHYSBASE0 ... IA32_MTRR_PHYSMASK7: {
        int index = (vctx->ecx - IA32_MTRR_PHYSBASE0) / 2;
        int is_mask = vctx->ecx % 2;
        if (is_mask) {
            result = mtrr_state.variable[index].mask;
        } else {
            result = mtrr_state.variable[index].base;
        }
        break;
    }
    case IA32_MTRR_FIX64K_00000:
        result = mtrr_state.fixed_64k;
        break;
    case IA32_MTRR_FIX16K_80000 ... IA32_MTRR_FIX16K_A0000: {
        int index = vctx->ecx - IA32_MTRR_FIX16K_80000;
        result = mtrr_state.fixed_16k[index];
        break;
    }
    case IA32_MTRR_FIX4K_C0000 ... IA32_MTRR_FIX4K_F8000: {
        int index = vctx->ecx - IA32_MTRR_FIX4K_C0000;
        result = mtrr_state.fixed_4k[index];
        break;
    }
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
    if (!msrs_initialised) {
        LOG_VMM_ERR("MSRs not initialised!\n");
        return false;
    }

    uint64_t value = (uint64_t)((vctx->edx & 0xffffffff) << 32) | (uint64_t)(vctx->eax & 0xffffffff);

    LOG_FAULT("handling WRMSR 0x%x, value 0x%lx\n", vctx->ecx, value);

    switch (vctx->ecx) {
    case MSR_EFER:
        microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_EFER, value);
        break;
    case IA32_PAT:
        microkit_vcpu_x86_write_vmcs(GUEST_BOOT_VCPU_ID, VMX_GUEST_PAT, value);
        break;
    case IA32_MTRR_DEF_TYPE:
        mtrr_state.mtrr_def_type = value;
        break;
    case IA32_MTRR_PHYSBASE0 ... IA32_MTRR_PHYSMASK7: {
        int index = (vctx->ecx - IA32_MTRR_PHYSBASE0) / 2;
        int is_mask = vctx->ecx % 2;
        if (is_mask) {
            mtrr_state.variable[index].mask = value;
        } else {
            mtrr_state.variable[index].base = value;
        }
        break;
    }
    case IA32_MTRR_FIX64K_00000:
        mtrr_state.fixed_64k = value;
        break;
    case IA32_MTRR_FIX16K_80000 ... IA32_MTRR_FIX16K_A0000: {
        int index = vctx->ecx - IA32_MTRR_FIX16K_80000;
        mtrr_state.fixed_16k[index] = value;
        break;
    }
    case IA32_MTRR_FIX4K_C0000 ... IA32_MTRR_FIX4K_F8000: {
        int index = vctx->ecx - IA32_MTRR_FIX4K_C0000;
        mtrr_state.fixed_4k[index] = value;
        break;
    }
    case MSR_STAR:
    case MSR_LSTAR:
    case MSR_CSTAR:
    case MSR_SYSCALL_MASK:
        microkit_vcpu_x86_write_msr(GUEST_BOOT_VCPU_ID, vctx->ecx, value);
        return true;
    case IA32_PRED_CMD:
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

    return true;
}
