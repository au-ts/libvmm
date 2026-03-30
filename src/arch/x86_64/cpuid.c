/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <sddf/util/util.h>
#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/cpuid.h>
#include <libvmm/arch/x86_64/util.h>

/* Documents referenced:
 * 1. Intel® 64 and IA-32 Architectures Software Developer’s Manual
 *    Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *    Order Number: 325462-080US June 2023
 * 2. https://en.wikipedia.org/wiki/X86-64#Microarchitecture_levels
 * 3. https://github.com/bochs-emu/Bochs/blob/master/bochs/cpu/cpudb/intel/corei7_skylake-x.txt
 */

/* 4 chars per 32-bit register, 4 registers per leaf, 3 leafs */
#define CPUID_BRAND_STR_LEN (4 * 4 * 3)
static const char brand_string[CPUID_BRAND_STR_LEN] = "Trustworthy Systems CPU";

extern uint64_t tsc_hz;

bool emulate_cpuid(seL4_VCPUContext *vctx)
{
    uint32_t a, b, c, d;
    LOG_FAULT("handling CPUID 0x%x\n", vctx->eax);

    switch (vctx->eax) {
    case 0x0: /* Basic CPUID Information */
        vctx->eax = CPUID_0H_EAX_MAX_BASIC_LEAF;
        vctx->ebx = CPUID_0H_GENUINEINTEL_EBX;
        vctx->edx = CPUID_0H_GENUINEINTEL_EDX;
        vctx->ecx = CPUID_0H_GENUINEINTEL_ECX;
        break;

    case 0x1:
        vctx->eax = CPUID_1H_EAX;
        vctx->ebx = CPUID_1H_EBX;
        vctx->ecx = CPUID_1H_X64_V2_BASELINE_ECX;
        vctx->edx = CPUID_1H_X64_V2_BASELINE_EDX;
        break;

    case 0x2: /* Cache and TLB Information */

    case 0x4: /* Deterministic Cache Parameters */
        /* Passthrough host's values */
        cpuid(vctx->eax, vctx->ecx, &a, &b, &c, &d);
        vctx->eax = a;
        vctx->ebx = b;
        vctx->ecx = c;
        vctx->edx = d;
        break;

    case 0x7: /* Structured Extended Feature Flags Enumeration */
        if (vctx->ecx == CPUID_7H_EAX_MAX_SUBLEAF && vctx->ecx == 0) {
            vctx->ebx = CPUID_7H_0_X64_V2_BASELINE_EBX;
        }
        vctx->eax = 0;
        vctx->ecx = 0;
        vctx->edx = 0;
        break;

    case 0xb: /* Extended Topology Enumeration Leaf */
        // @billn revisit for multiple VCPUs
        vctx->eax = 0;
        vctx->ebx = 0;
        vctx->ecx = vctx->ecx;
        vctx->edx = 0; // x2apic id, though we don't use x2apic
        break;

    // sus @billn revisit
    case 0xd: {
        uint32_t ecx = vctx->ecx;
        cpuid(0xd, vctx->ecx, (uint32_t *)&vctx->eax, (uint32_t *)&vctx->ebx, (uint32_t *)&vctx->ecx,
              (uint32_t *)&vctx->edx);
        if (ecx == 0) {
            // TODO: this is a complete hack because we know seL4 will set the XCR
            // to 0x3.
            vctx->eax = 0x3;
        }
        if (ecx == 1) {
#if !defined(CONFIG_XSAVE_XSAVEC)
            if (vctx->eax & BIT(1)) {
                vctx->eax &= ~BIT(1);
                // LOG_VMM("XSAVEC is available in CPU but not available in seL4, disabling for guest\n");
            }
#endif
#if !defined(CONFIG_XSAVE_XSAVEOPT)
            if (vctx->eax & BIT(0)) {
                vctx->eax &= ~BIT(0);
                // LOG_VMM("XSAVEOPT is available in CPU but not available in seL4, disabling for guest\n");
            }
#endif
#if !defined(CONFIG_XSAVE_XSAVES)
            if (vctx->eax & BIT(3)) {
                vctx->eax &= ~BIT(3);
                // LOG_VMM("XSAVES is available in CPU but not available in seL4, disabling for guest\n");
            }
#endif
        }
        break;
    }

    case 0x15: /* Time Stamp Counter and Nominal Core Crystal Clock */
        vctx->eax = 1; /* Making ratio between Crystal and TSC 1-to-1 */
        vctx->ebx = 1;
        vctx->ecx = tsc_hz;
        vctx->edx = 0;
        break;

    case 0x16: /* Processor Frequency Information Leaf */
        /* CPU base, max and bus frequency in MHz */
        vctx->eax = tsc_hz / 1000000;
        vctx->ebx = tsc_hz / 1000000;
        vctx->ecx = tsc_hz / 1000000;
        vctx->edx = 0;
        break;

    case 0x80000000: /* Extended Function CPUID Information */
        vctx->eax = CPUID_80000000H_EAX_MAX_EXT_LEAF; /* max input value for extended function cpuid information */
        vctx->ebx = 0;
        vctx->ecx = 0;
        vctx->edx = 0;
        break;
    case 0x80000001: /* Extended Processor Signature and Feature Bits */
        vctx->eax = 0;
        vctx->ebx = 0;
        vctx->ecx = CPUID_80000001H_X64_V2_BASELINE_ECX;
        vctx->edx = CPUID_80000001H_X64_V2_BASELINE_EDX;
        if (guest_in_64_bits()) {
            /* [1] "This feature flag is always enumerated as 0 outside 64-bit mode." */
            vctx->edx |= CPUID_80000001_EDX_SCALL_SRET;
        }
        break;

    case 0x80000002: /* Processor Brand String. */
        memcpy(&(vctx->eax), brand_string, 4);
        memcpy(&(vctx->ebx), brand_string + 4, 4);
        memcpy(&(vctx->ecx), brand_string + 8, 4);
        memcpy(&(vctx->edx), brand_string + 12, 4);
        break;
    case 0x80000003:
        memcpy(&(vctx->eax), brand_string + 16, 4);
        memcpy(&(vctx->ebx), brand_string + 20, 4);
        memcpy(&(vctx->ecx), brand_string + 24, 4);
        memcpy(&(vctx->edx), brand_string + 28, 4);
        break;
    case 0x80000004:
        memcpy(&(vctx->eax), brand_string + 32, 4);
        memcpy(&(vctx->ebx), brand_string + 36, 4);
        memcpy(&(vctx->ecx), brand_string + 40, 4);
        memcpy(&(vctx->edx), brand_string + 44, 4);
        break;

    case 0x80000006: /* L2 cache topology */
    case 0x80000007: /* Invariant TSC */
    case 0x80000008: /* Physical Address size */
        /* Passthrough host's values */
        cpuid(vctx->eax, vctx->ecx, &a, &b, &c, &d);
        vctx->eax = a;
        vctx->ebx = b;
        vctx->ecx = c;
        vctx->edx = d;
        break;

    case 0x3:
    case 0x5:
    case 0x6:
    case 0x8:
    case 0x9:
    case 0xa:
    case 0xc:
    case 0xe:
    case 0xf:
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14:
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1a:
    case 0x1b:
    case 0x1c:
    case 0x1d:
    case 0x1e:
    case 0x1f:
    case 0x21:
    case 0x40000000 ... 0x4fffffff:
    case 0x80000005:
    /* some AMD specific stuff beyond 0x80000009 inclusive */
    case 0x80000009 ... 0x8000001f:
    case 0x80000026:
    /* Highest Xeon Phi Function Implemented */
    case 0x20000000:
    /* Transmeta */
    case 0x80860000:
    /* Highest Centaur Extended Function */
    case 0xc0000000:
    /* Zhaoxin Feature Information */
    case 0xc0000006:
        vctx->eax = 0;
        vctx->ebx = 0;
        vctx->ecx = 0;
        vctx->edx = 0;
        break;
    default:
        LOG_VMM_ERR("invalid cpuid eax value: 0x%x, returning zero\n", vctx->eax);
        vctx->eax = 0;
        vctx->ebx = 0;
        vctx->ecx = 0;
        vctx->edx = 0;
        break;
    }
    return true;
}