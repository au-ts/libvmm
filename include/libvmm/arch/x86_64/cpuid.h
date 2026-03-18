/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <microkit.h>

/* Documents referenced:
 * 1. Intel® 64 and IA-32 Architectures Software Developer’s Manual
 *    Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *    Order Number: 325462-080US June 2023
 * 2. https://en.wikipedia.org/wiki/X86-64#Microarchitecture_levels
 * 3. https://github.com/bochs-emu/Bochs/blob/master/bochs/cpu/cpudb/intel/corei7_skylake-x.txt
 */

#define CACHE_LINE_SIZE 64
#define NUM_LOGICAL_PROCESSORS 1

/* [1] Table 3-8. Information Returned by CPUID Instruction */
#define CPUID_0H_EAX_MAX_BASIC_LEAF 0x16
#define CPUID_0H_GENUINEINTEL_EBX 0x756e6547 /* "GenuineIntel" */
#define CPUID_0H_GENUINEINTEL_EDX 0x49656e69
#define CPUID_0H_GENUINEINTEL_ECX 0x6c65746e

#define CPUID_1H_EAX 0x00050054 /* [3] Intel Core i7-7800X Skylake */
#define CPUID_1H_EBX ((CACHE_LINE_SIZE / 8) << 8) | (NUM_LOGICAL_PROCESSORS << 16)

/* [1] Table 3-10. More on Feature Information Returned in the ECX Register */
#define CPUID_1H_ECX_SSE3 BIT(0)
#define CPUID_1H_ECX_PCLMULQDQ BIT(1)
#define CPUID_1H_ECX_SSSE3 BIT(9)
#define CPUID_1H_ECX_FMA BIT(12)
#define CPUID_1H_ECX_CMPXCHG16B BIT(13)
#define CPUID_1H_ECX_SSE4_1 BIT(19)
#define CPUID_1H_ECX_SSE4_2 BIT(20)
#define CPUID_1H_ECX_MOVBE BIT(22)
#define CPUID_1H_ECX_POPCNT BIT(23)
#define CPUID_1H_ECX_AES BIT(25)
#define CPUID_1H_ECX_XSAVE BIT(26)
#define CPUID_1H_ECX_OSXSAVE BIT(27)
#define CPUID_1H_ECX_AVX BIT(28)
#define CPUID_1H_ECX_F16C BIT(29)
#define CPUID_1H_ECX_RDRAND BIT(30)

/* [1] Table 3-11. More on Feature Information Returned in the EDX Register */
#define CPUID_1H_EDX_FPU BIT(0)
#define CPUID_1H_EDX_VME BIT(1)  // VME: Virtual-8086 Mode enhancements
#define CPUID_1H_EDX_DE BIT(2)   // DE: Debug Extensions (I/O breakpoints)
#define CPUID_1H_EDX_PSE BIT(3)  // PSE: Page Size Extensions
#define CPUID_1H_EDX_TSC BIT(4)
#define CPUID_1H_EDX_MSR BIT(5)
#define CPUID_1H_EDX_PAE BIT(6)
#define CPUID_1H_EDX_CX8 BIT(8)
#define CPUID_1H_EDX_APIC BIT(9)
#define CPUID_1H_EDX_MTRR BIT(12)
#define CPUID_1H_EDX_PGE BIT(13) // PGE/PTE Global Bit
#define CPUID_1H_EDX_CMOV BIT(15)
#define CPUID_1H_EDX_PAT BIT(16)
#define CPUID_1H_EDX_PSE36 BIT(17) // PSE-36: Physical Address Extensions
#define CPUID_1H_EDX_CLFLUSH BIT(19)
#define CPUID_1H_EDX_MMX BIT(23)
#define CPUID_1H_EDX_FXSR BIT(24)
#define CPUID_1H_EDX_SSE1 BIT(25)
#define CPUID_1H_EDX_SSE2 BIT(26)
#define CPUID_1H_EDX_SELF_SNOOP BIT(27)

/* [1] Table 3-8. Information Returned by CPUID Instruction */
#define CPUID_7H_EAX_MAX_SUBLEAF 0

#define CPUID_7H_00_EBX_FSGSBASE BIT(0)
#define CPUID_7H_00_EBX_BMI1 BIT(3)
#define CPUID_7H_00_EBX_SMEP BIT(7) // SMEP: Supervisor Mode Execution Protection
#define CPUID_7H_00_EBX_BMI2 BIT(8)
#define CPUID_7H_00_EBX_DEPRECATE_FCS_FDS BIT(13) // Deprecates FPU CS and FPU DS values
#define CPUID_7H_00_EBX_ADX BIT(19) // ADCX/ADOX instructions support
#define CPUID_7H_00_EBX_SMAP BIT(20) // SMAP: Supervisor Mode Access Prevention
#define CPUID_7H_00_EBX_CLFLUSHOPT BIT(23)

#define CPUID_80000000H_EAX_MAX_EXT_LEAF 0x80000008

#define CPUID_80000001_ECX_LM_LAHF_SAHF BIT(0) // LAHF/SAHF instructions support in 64-bit mode
#define CPUID_80000001_ECX_LM_LZCNT BIT(5) // LZCNT: LZCNT instruction support
#define CPUID_80000001_ECX_PREFETCHW BIT(8)

#define CPUID_80000001_EDX_SCALL_SRET BIT(11) // SYSCALL/SYSRET support
#define CPUID_80000001_EDX_NX BIT(20) // No execute
#define CPUID_80000001_EDX_1GB_PAGE BIT(26)
#define CPUID_80000001_EDX_LONG_MODE BIT(29)

/* [2] x86-64_v2 baseline CPUID, which modern Linux distros and Windows 10 requires. */

/* No:
 * qualified debug store
 * Intel Enhanced SpeedStep
 * Thermal Monitor 2
 * Perfmon
 * MONITOR MWAIT
 * VMX
 * PCID, seL4 in Microkit isn't built with it
 * x2APIC
 * TSC deadline mode for LAPIC timer
 */
#define CPUID_1H_X64_V2_BASELINE_ECX ( \
    CPUID_1H_ECX_SSE3 | \
    CPUID_1H_ECX_PCLMULQDQ | \
    CPUID_1H_ECX_SSSE3 | \
    CPUID_1H_ECX_FMA | \
    CPUID_1H_ECX_CMPXCHG16B | \
    CPUID_1H_ECX_SSE4_1 | \
    CPUID_1H_ECX_SSE4_2 | \
    CPUID_1H_ECX_MOVBE | \
    CPUID_1H_ECX_POPCNT | \
    CPUID_1H_ECX_AES | \
    CPUID_1H_ECX_XSAVE | \
    CPUID_1H_ECX_OSXSAVE | \
    CPUID_1H_ECX_AVX | \
    CPUID_1H_ECX_F16C | \
    CPUID_1H_ECX_RDRAND \
)

/* No:
 * Debug Store
 * ACPI Thermal Monitor
 * Thermal Monitor
 * virtual 8086
 * MTRR
 * MCE
 */
#define CPUID_1H_X64_V2_BASELINE_EDX ( \
    CPUID_1H_EDX_FPU | \
    CPUID_1H_EDX_VME | \
    CPUID_1H_EDX_PAT | \
    CPUID_1H_EDX_MTRR | \
    CPUID_1H_EDX_DE | \
    CPUID_1H_EDX_PSE | \
    CPUID_1H_EDX_TSC | \
    CPUID_1H_EDX_MSR | \
    CPUID_1H_EDX_PAE | \
    CPUID_1H_EDX_CX8 | \
    CPUID_1H_EDX_APIC | \
    CPUID_1H_EDX_PGE | \
    CPUID_1H_EDX_CMOV | \
    CPUID_1H_EDX_PSE36 | \
    CPUID_1H_EDX_CLFLUSH | \
    CPUID_1H_EDX_MMX | \
    CPUID_1H_EDX_FXSR | \
    CPUID_1H_EDX_SSE1 | \
    CPUID_1H_EDX_SSE2 | \
    CPUID_1H_EDX_SELF_SNOOP \
)

/* No:
 * TSC adjust
 * AVX512
 * INVPCID
 * RDSEED
 */
#define CPUID_7H_0_X64_V2_BASELINE_EBX ( \
    CPUID_7H_00_EBX_FSGSBASE | \
    CPUID_7H_00_EBX_BMI1 | \
    CPUID_7H_00_EBX_SMEP | \
    CPUID_7H_00_EBX_BMI2 | \
    CPUID_7H_00_EBX_DEPRECATE_FCS_FDS | \
    CPUID_7H_00_EBX_ADX | \
    CPUID_7H_00_EBX_SMAP | \
    CPUID_7H_00_EBX_CLFLUSHOPT \
)

#define CPUID_80000001H_X64_V2_BASELINE_ECX ( \
    CPUID_80000001_ECX_LM_LAHF_SAHF | \
    CPUID_80000001_ECX_LM_LZCNT | \
    CPUID_80000001_ECX_PREFETCHW \
)

/* No RDTSCP */
#define CPUID_80000001H_X64_V2_BASELINE_EDX ( \
    CPUID_80000001_EDX_NX | \
    CPUID_80000001_EDX_1GB_PAGE | \
    CPUID_80000001_EDX_LONG_MODE \
)

bool initialise_cpuid(void);

bool emulate_cpuid(seL4_VCPUContext *vctx);
