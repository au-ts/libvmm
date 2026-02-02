/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sddf/util/util.h>
#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/cpuid.h>
#include <libvmm/arch/x86_64/util.h>

// https://github.com/bochs-emu/Bochs/blob/master/bochs/cpu/cpudb/intel/corei7_skylake-x.cc

#define CACHE_LINE_SIZE 64
#define NUM_LOGICAL_PROCESSORS 1

extern uint64_t tsc_hz;

// Table 3-11. More on Feature Information Returned in the EDX Register
#define CPUID_01_ECX_SSE3 BIT(0)
#define CPUID_01_ECX_PCLMULQDQ BIT(1)
#define CPUID_01_ECX_DTES64 BIT(2)     // DTES64: 64-bit DS area
#define CPUID_01_ECX_DS_CPL BIT(4)     // DS-CPL: CPL qualified debug store
#define CPUID_01_ECX_EST BIT(7)        // EST: Enhanced Intel SpeedStep Technology
#define CPUID_01_ECX_TM2 BIT(8)        // TM2: Thermal Monitor 2
#define CPUID_01_ECX_SSSE3 BIT(9)
#define CPUID_01_ECX_FMA BIT(12)
#define CPUID_01_ECX_CMPXCHG16B BIT(13)
#define CPUID_01_ECX_XTPR BIT(14)      // xTPR update control
#define CPUID_01_ECX_PDCM BIT(15)      // PDCM - Perfmon and Debug Capability MSR
#define CPUID_01_ECX_SSE4_1 BIT(19)
#define CPUID_01_ECX_SSE4_2 BIT(20)
#define CPUID_01_ECX_MOVBE BIT(22)
#define CPUID_01_ECX_POPCNT BIT(23)
#define CPUID_01_ECX_AES BIT(25)
#define CPUID_01_ECX_XSAVE BIT(26)
#define CPUID_01_ECX_OSXSAVE BIT(27)
#define CPUID_01_ECX_AVX BIT(28)
#define CPUID_01_ECX_F16C BIT(29)
#define CPUID_01_ECX_RDRAND BIT(30)
#define CPUID_01_ECX_HYP BIT(31) // Running on hypervisor, correspond to X86_FEATURE_HYPERVISOR in linux source

#define CPUID_01_EDX_FPU BIT(0)
#define CPUID_01_EDX_VME BIT(1)  // VME: Virtual-8086 Mode enhancements
#define CPUID_01_EDX_DE BIT(2)   // DE: Debug Extensions (I/O breakpoints)
#define CPUID_01_EDX_PSE BIT(3)  // PSE: Page Size Extensions
#define CPUID_01_EDX_TSC BIT(4)
#define CPUID_01_EDX_MSR BIT(5)
#define CPUID_01_EDX_PAE BIT(6)
#define CPUID_01_EDX_MCE BIT(7)  // MCE: Machine Check Exception
#define CPUID_01_EDX_CX8 BIT(8)
#define CPUID_01_EDX_APIC BIT(9)
#define CPUID_01_EDX_MTRR BIT(12) // MTRR: Memory Type Range Reg
#define CPUID_01_EDX_PGE BIT(13) // PGE/PTE Global Bit
#define CPUID_01_EDX_MCA BIT(14) // MCA: Machine Check Architecture
#define CPUID_01_EDX_CMOV BIT(15)
#define CPUID_01_EDX_PAT BIT(16)
#define CPUID_01_EDX_PSE36 BIT(17) // PSE-36: Physical Address Extensions
#define CPUID_01_EDX_DEBUG_STORE BIT(21)
#define CPUID_01_EDX_ACPI BIT(22) // ACPI: Thermal Monitor and Software Controlled Clock Facilities
#define CPUID_01_EDX_MMX BIT(23)
#define CPUID_01_EDX_FXSR BIT(24)
#define CPUID_01_EDX_SSE1 BIT(25)
#define CPUID_01_EDX_SSE2 BIT(26)
#define CPUID_01_EDX_SELF_SNOOP BIT(27)
#define CPUID_01_EDX_TM BIT(29) // TM: Thermal Monitor
#define CPUID_01_EDX_PBE BIT(31) // PBE: Pending Break Enable

#define CPUID_07_00_EBX_FSGSBASE BIT(0)
#define CPUID_07_00_EBX_BMI1 BIT(3)
#define CPUID_07_00_EBX_AVX2 BIT(5)
#define CPUID_07_00_EBX_SMEP BIT(7) // SMEP: Supervisor Mode Execution Protection
#define CPUID_07_00_EBX_BMI2 BIT(8)
#define CPUID_07_00_EBX_DEPRECATE_FCS_FDS BIT(13) // Deprecates FPU CS and FPU DS values
#define CPUID_07_00_EBX_ADX BIT(19) // ADCX/ADOX instructions support
#define CPUID_07_00_EBX_SMAP BIT(20) // SMAP: Supervisor Mode Access Prevention
#define CPUID_07_00_EBX_CLFLUSHOPT BIT(23)
#define CPUID_07_00_EBX_CLWB BIT(24)

#define CPUID_80000001_ECX_LM_LAHF_SAHF BIT(0) // LAHF/SAHF instructions support in 64-bit mode
#define CPUID_80000001_ECX_LM_LZCNT BIT(5) // LZCNT: LZCNT instruction support
#define CPUID_80000001_ECX_PREFETCHW BIT(8)

#define CPUID_80000001_EDX_SCALL_SRET BIT(11) // SYSCALL/SYSRET support
#define CPUID_80000001_EDX_NX BIT(20) // No execute
#define CPUID_80000001_EDX_1GB_PAGE BIT(26)
#define CPUID_80000001_EDX_RDTSCP BIT(27)
#define CPUID_80000001_EDX_LONG_MODE BIT(29)

// TODO: do this instead
// #define CPUID_01_BASELINE_EDX (CPUID_01_EDX_CMOV |
//                                CPUID_01_EDX_CX8 |
//                                CPUID_01_EDX_FPU |
//                                CPUID_01_EDX_FXSR |
//                                CPUID_01_EDX_MMX |
//                                CPUID_01_EDX_SSE1 |
//                                CPUID_01_EDX_SSE2)

// #define CPUID_01_X86_64_V2_EDX (CPUID_01_ECX_CMPXCHG16B |
//                                 CPUID_01_ECX_LAHF_SAHF |

//                                 )

static const char brand_string[48] = "Trustworthy Systems CPU";

static inline void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
    __asm__ __volatile__("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "a"(leaf), "c"(subleaf));
}

bool emulate_cpuid(seL4_VCPUContext *vctx)
{
    LOG_FAULT("handling CPUID 0x%x\n", vctx->eax);

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
        vctx->eax = 0x00050054;
        vctx->ebx = ((CACHE_LINE_SIZE / 8) << 8) | (NUM_LOGICAL_PROCESSORS << 16);
        vctx->ecx = CPUID_01_ECX_DTES64 | 0 /* No qualified debug store */
                  | 0 /* No Intel Enhanced SpeedStep */ | 0 /* No Thermal Monitor 2 */ | CPUID_01_ECX_XTPR
                  | 0 /* No Perfmon */ | CPUID_01_ECX_SSE3 | CPUID_01_ECX_PCLMULQDQ
                  | 0 /* No MONITOR MWAIT*/ | 0 /* No VMX*/ | CPUID_01_ECX_SSSE3 | CPUID_01_ECX_FMA
                  | CPUID_01_ECX_CMPXCHG16B
                  | 0 /* No PCID, seL4 in Microkit isn't built with it */ | CPUID_01_ECX_SSE4_1 | CPUID_01_ECX_SSE4_2
                  | 0 /* No x2APIC */ | CPUID_01_ECX_MOVBE | CPUID_01_ECX_POPCNT
                  | 0 /* No TSC deadline */ | CPUID_01_ECX_AES | CPUID_01_ECX_XSAVE | CPUID_01_ECX_OSXSAVE
                  | CPUID_01_ECX_AVX | CPUID_01_ECX_F16C | 0 /* No RDRAND */ | CPUID_01_ECX_HYP;

        vctx->edx = 0 /* No Debug Store */ | 0 /* No ACPI Thermal Monitor */ | CPUID_01_EDX_SELF_SNOOP
                  | 0/* No THermal Monitor */
                  | CPUID_01_EDX_PBE | CPUID_01_EDX_FPU | 0 /* No virtual 8086 mode */ | 0 /* No Debug Extension */
                  | CPUID_01_EDX_PSE | CPUID_01_EDX_TSC | CPUID_01_EDX_MSR | CPUID_01_EDX_PAE
                  | 0 /* No Machine Check Exception */
                  | CPUID_01_EDX_CX8 | CPUID_01_EDX_APIC | 0 /* No MTRR */ | CPUID_01_EDX_PGE
                  | 0 /* No Machine Check Architecture */
                  | CPUID_01_EDX_CMOV | 0 /* No PAT */ | CPUID_01_EDX_PSE36 | CPUID_01_EDX_MMX | CPUID_01_EDX_FXSR
                  | CPUID_01_EDX_SSE2 | CPUID_01_EDX_SSE1;

        break;
    }

    case 0x2: {
        // Cache and TLB description
        vctx->eax = 0x76036301;
        vctx->ebx = 0x00f0b5ff;
        vctx->ecx = 0x00000000;
        vctx->edx = 0x00c30000;
        break;
    }

    case 0x4: {
        // Deterministic Cache Parameters
        switch (vctx->ecx) {
        case 0:
            vctx->eax = 0x1c004121;
            vctx->ebx = 0x01c0003f;
            vctx->ecx = 0x0000003f;
            vctx->edx = 0x00000000;
            break;
        case 1:
            vctx->eax = 0x1c004122;
            vctx->ebx = 0x01c0003f;
            vctx->ecx = 0x0000003f;
            vctx->edx = 0x00000000;
            break;
        case 2:
            vctx->eax = 0x1c004143;
            vctx->ebx = 0x03c0003f;
            vctx->ecx = 0x000003ff;
            vctx->edx = 0x00000000;
            break;
        case 3:
            vctx->eax = 0x1c03c163;
            vctx->ebx = 0x0280003f;
            vctx->ecx = 0x00002fff;
            vctx->edx = 0x00000004;
            break;
        default:
            vctx->eax = 0;
            vctx->ebx = 0;
            vctx->ecx = 0;
            vctx->edx = 0;
            break;
        }
        break;
    }

    case 0x7: {
        if (vctx->ecx == 0) {
            vctx->eax = 0;
            vctx->ebx = CPUID_07_00_EBX_FSGSBASE | CPUID_07_00_EBX_BMI1 | CPUID_07_00_EBX_BMI2
                      | 0 /* No TSC adjust */ | 0 /* No AVX2 */ | CPUID_07_00_EBX_SMEP
                      | 0 /* No INVPCID */ | CPUID_07_00_EBX_DEPRECATE_FCS_FDS
                      | 0 /* No AVX512 */ | 0 /* No RDSEED */ | CPUID_07_00_EBX_ADX | CPUID_07_00_EBX_SMAP
                      | CPUID_07_00_EBX_CLFLUSHOPT | CPUID_07_00_EBX_CLWB;
            vctx->ecx = 0;
            vctx->edx = 0;
        } else {
            vctx->eax = 0;
            vctx->ebx = 0;
            vctx->ecx = 0;
            vctx->edx = 0;
        }
        break;
    }

    case 0xb: {
        vctx->eax = 0;
        vctx->ebx = 0;
        vctx->ecx = vctx->ecx;
        vctx->edx = 0; // x2apic id, weird because we don't use x2apic
        break;
    }

    // sus
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
    case 0x15: {
        // Time Stamp Counter and Nominal Core Crystal Clock
        vctx->eax = 1;
        vctx->ebx = 1;
        vctx->ecx = tsc_hz;
        vctx->edx = 0;
        break;
    }
    case 0x16:
        // Table 3-8. Information Returned by CPUID Instruction (Contd.)
        // page "3-232 Vol. 2A"
        // processor and bus clock in MHz
        // Linux can measure this itself but it needs the PIT,
        // which we aren't emulating.
        vctx->eax = tsc_hz / 1000000;
        vctx->ebx = tsc_hz / 1000000;
        vctx->ecx = tsc_hz / 1000000;
        vctx->edx = 0;
        break;
    case 0x80000000:
        vctx->eax = 0x80000008; // max input value for extended function cpuid information
        vctx->ebx = 0;
        vctx->ecx = 0;
        vctx->edx = 0;
        break;
    case 0x80000001:
        vctx->eax = 0;
        vctx->ebx = 0;
        vctx->ecx = CPUID_80000001_ECX_LM_LAHF_SAHF | CPUID_80000001_ECX_LM_LZCNT | CPUID_80000001_ECX_PREFETCHW;
        vctx->edx = 0 /* No RDTSCP */ | CPUID_80000001_EDX_NX
                  | CPUID_80000001_EDX_LONG_MODE | CPUID_80000001_EDX_1GB_PAGE;
        if (guest_in_64_bits()) {
            vctx->edx |= CPUID_80000001_EDX_SCALL_SRET;
        }
        break;

    case 0x80000002:
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

    case 0x80000006:
        // L2 cache topology
        vctx->eax = 0;
        vctx->ebx = 0;
        vctx->ecx = 0x01006040;
        vctx->edx = 0;
        break;

    case 0x80000007:
        vctx->eax = 0;
        vctx->ebx = 0;
        vctx->ecx = 0;
        // @billn should validate that the host also have this, otherwise the guest clock will be off.
        vctx->edx = BIT(8); // invariant TSC, ticks at same rate regardless of power state
        break;

    case 0x80000008:
        cpuid(vctx->eax, vctx->ecx, (uint32_t *)&vctx->eax, (uint32_t *)&vctx->ebx, (uint32_t *)&vctx->ecx,
              (uint32_t *)&vctx->edx);
        break;

    case 0x40000000:
        // KVM
        vctx->eax = 0x40000001;
        vctx->ebx = 0x4b4d564b;
        vctx->ecx = 0x564b4d56;
        vctx->edx = 0x4d;
        break;

    case 0x40000001:
        // KVM
        vctx->eax = BIT(1); // no io port access delay
        vctx->ebx = 0;
        vctx->ecx = 0;
        vctx->edx = 0;
        break;

    // @billn todo double check if these are needed
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
    case 0x40000002 ... 0x4fffffff:
    case 0x80000005:
    // some AMD specific stuff beyond 0x80000009 inclusive
    case 0x80000009 ... 0x8000001f:
    case 0x80000026:
        vctx->eax = 0;
        vctx->ebx = 0;
        vctx->ecx = 0;
        vctx->edx = 0;
        break;
    default:
        LOG_VMM_ERR("invalid cpuid eax value: 0x%x\n", vctx->eax);
        return false;
    }
    return true;
}