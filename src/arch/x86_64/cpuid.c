/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sddf/util/util.h>
#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/cpuid.h>

extern uint64_t tsc_hz;

// Table 3-11. More on Feature Information Returned in the EDX Register
#define CPUID_01_EDX_FPU BIT(0)
#define CPUID_01_EDX_TSC BIT(4)
#define CPUID_01_EDX_MSR BIT(5)
#define CPUID_01_EDX_PAE BIT(6)
#define CPUID_01_EDX_APIC BIT(9)
#define CPUID_01_EDX_SSE1 BIT(25)
#define CPUID_01_EDX_SSE2 BIT(26)

#define CPUID_01_ECX_SSE3 BIT(0)
#define CPUID_01_ECX_SSSE3 BIT(9)
#define CPUID_01_ECX_FMA BIT(12)
#define CPUID_01_ECX_CMPXCHG16B BIT(13)
#define CPUID_01_ECX_SSE4_1 BIT(19)
#define CPUID_01_ECX_SSE4_2 BIT(20)
#define CPUID_01_ECX_MOVBE BIT(22)
#define CPUID_01_ECX_POPCNT BIT(23)
#define CPUID_01_ECX_XSAVE BIT(26)
#define CPUID_01_ECX_OSXSAVE BIT(27)
#define CPUID_01_ECX_AVX BIT(28)
#define CPUID_01_ECX_F16C BIT(29)

#define CPUID_07_00_EBX_BMI1 BIT(3)
#define CPUID_07_00_EBX_AVX2 BIT(5)
#define CPUID_07_00_EBX_BMI2 BIT(8)

static inline void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
    __asm__ __volatile__("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "a"(leaf), "c"(subleaf));
}

bool emulate_cpuid(seL4_VCPUContext *vctx)
{
    // @billn todo revisit likely need to turn on some important features.
    // 3-218 Vol. 2A

    switch (vctx->eax) {
    case 0x0:
            // 3-240 Vol. 2A
            // GenuineIntel
        vctx->eax = 0x16;
        vctx->ebx = 0x756e6547;
        vctx->edx = 0x49656e69;
        vctx->ecx = 0x6c65746e;
        break;
    case 0x1: {
            // Encoding from:
            // https://en.wikipedia.org/wiki/CPUID
            // "EAX=1: Processor Info and Feature Bits"

            // Values from:
            // https://en.wikichip.org/wiki/intel/cpuid

            // OEM processor: bit 12 and 13 zero
        uint32_t model_id = 0xe << 4; // skylake client
        uint32_t ext_model_id = 0x5 << 16;
            // Pentium and Intel Core family
        uint32_t family_id = 0x6 << 8;

        vctx->eax = model_id | ext_model_id | family_id;

            /* Table 1-20. */
        vctx->ebx = 0;
            // vctx->ecx = BIT(24); // TSC deadline supported
        vctx->ecx = CPUID_01_ECX_SSE3 | CPUID_01_ECX_SSSE3 | CPUID_01_ECX_XSAVE | CPUID_01_ECX_CMPXCHG16B
                  | CPUID_01_ECX_SSE4_1 | CPUID_01_ECX_SSE4_2 | CPUID_01_ECX_POPCNT | CPUID_01_ECX_OSXSAVE
                  | CPUID_01_ECX_AVX | CPUID_01_ECX_FMA | CPUID_01_ECX_F16C | CPUID_01_ECX_MOVBE;
        vctx->edx = CPUID_01_EDX_TSC | CPUID_01_EDX_MSR | CPUID_01_EDX_PAE | CPUID_01_EDX_APIC | CPUID_01_EDX_FPU
                  | CPUID_01_EDX_SSE1 | CPUID_01_EDX_SSE2;
        break;
    }
    case 0x2:
    case 0x3:
    case 0x4:
    case 0x5:
    case 0x6:
    case 0x7:
        vctx->eax = 0;
        vctx->ebx = 0;
        vctx->ecx = 0;
        vctx->edx = 0;
        if (vctx->ecx == 0) {
            vctx->ebx = CPUID_07_00_EBX_BMI1 | CPUID_07_00_EBX_AVX2 | CPUID_07_00_EBX_BMI2;
        }
        break;
    case 0x9:
    case 0xa:
    case 0xb:
    case 0xd: {
        uint32_t ecx = vctx->ecx;
        cpuid(0xd, vctx->ecx, &vctx->eax, &vctx->ebx, &vctx->ecx, &vctx->edx);
#if !defined(CONFIG_XSAVE_XSAVEC)
        if (ecx == 1 && vctx->eax & BIT(1)) {
            uint32_t eax = vctx->eax;
            vctx->eax &= ~BIT(1);
            LOG_VMM("XSAVEC is available in CPU but not available in seL4, disabling for guest\n");
        }
#endif
#if !defined(CONFIG_XSAVE_XSAVEOPT)
        if (ecx == 1 && vctx->eax & BIT(0)) {
            uint32_t eax = vctx->eax;
            vctx->eax &= ~BIT(0);
            LOG_VMM("XSAVEOPT is available in CPU but not available in seL4, disabling for guest\n");
        }
#endif
#if !defined(CONFIG_XSAVE_XSAVES)
        if (ecx == 1 && vctx->eax & BIT(3)) {
            uint32_t eax = vctx->eax;
            vctx->eax &= ~BIT(3);
            LOG_VMM("XSAVES is available in CPU but not available in seL4, disabling for guest\n");
        }
#endif
        break;
    }
    case 0xf:
    case 0x10:
    case 0x12:
    case 0x14:
    case 0x15:
    case 0x21:
    case 0x40000000 ... 0x4fffffff:
    case 0x8000001f:
        vctx->eax = 0;
        vctx->ebx = 0;
        vctx->ecx = 0;
        vctx->edx = 0;
        break;
    case 0x16:
            // Table 3-8. Information Returned by CPUID Instruction (Contd.)
            // page "3-232 Vol. 2A"
            // processor and bus clock in MHz
            // Linux can measure this itself but it needs the PIT,
            // which we aren't emulating.
        vctx->eax = tsc_hz / 1000000;
        vctx->ebx = tsc_hz / 1000000;
        vctx->ecx = tsc_hz / 1000000;
        break;
    case 0x80000000:
        vctx->eax = 1;
        break;
    case 0x80000001:
        vctx->eax = 0;
        vctx->ecx = BIT(0) | BIT(5); // LAHF/SAHF in long mode (LAHF_LM) + LZCNT
        vctx->edx = (1 << 11) | (1 << 29); // SYSCALL/SYSRET + Intel® 64
        break;
    default:
        LOG_VMM_ERR("invalid cpuid eax value: 0x%x\n", vctx->eax);
        return false;
    }
    return true;
}