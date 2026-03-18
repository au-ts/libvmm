/*
 * Copyright 2026, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <microkit.h>
#include <sddf/util/util.h>
#include <sddf/timer/client.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/tsc.h>
#include <libvmm/arch/x86_64/cpuid.h>

/* Uncomment this to enable debug logging */
#define DEBUG_TSC

#if defined(DEBUG_TSC)
#define LOG_TSC(...) do{ printf("%s|TSC: ", microkit_name); printf(__VA_ARGS__); } while(0)
#else
#define LOG_TSC(...) do{}while(0)
#endif

/* Documents referenced:
 * 1. Intel® 64 and IA-32 Architectures Software Developer’s Manual
 *    Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *    Order Number: 325462-080US June 2023
 * 2. Linux v6.17 source: native_calibrate_tsc() arch/x86/kernel/tsc.c
 */

/* CPUID related definitions for TSC detection. */

/* [1] "Vendor Identification String" page "3-240 Vol. 2A" */
#define CPUID_VENDOR_ID_LEAF 0x0

/* [1] "Time Stamp Counter and Nominal Core Crystal Clock Information Leaf" page "Vol. 2A 3-231" */
#define CPUID_TSC_LEAF 0x15

/* [1] "Processor Frequency Information Leaf" page "3-232 Vol. 2A" */
#define CPUID_PROC_FREQ_LEAF 0x16

/* [1] "Maximum Input Value for Extended Function CPUID Information." page "Vol. 2A 3-245" */
#define CPUID_MAX_EXT_LEAF 0x80000000

/* [1] "Invariant TSC available if 1" page "Vol. 2A 3-239" */
#define CPUID_INVARIANT_TSC_LEAF 0x80000007
#define CPUID_INVARIANT_TSC_EDX_BIT 8

static bool is_intel_cpu(void)
{
    uint32_t a, b, c, d;
    cpuid(CPUID_VENDOR_ID_LEAF, 0, &a, &b, &c, &d);
    return b == CPUID_0H_GENUINEINTEL_EBX && d == CPUID_0H_GENUINEINTEL_EDX && c == CPUID_0H_GENUINEINTEL_ECX;
}

static uint64_t get_tsc_frequency(void)
{
    uint32_t max_basic_leaf, b, c, d;
    /* Checks whether the CPU expose TSC/Crystal ratio and Crystal frequency via cpuid leaf 0x15. */
    cpuid(CPUID_VENDOR_ID_LEAF, 0, &max_basic_leaf, &b, &c, &d);
    if (max_basic_leaf < CPUID_TSC_LEAF) {
        LOG_TSC("CPU does not expose TSC leaf.\n");
        return 0;
    }

    uint32_t denominator, numerator, crystal_khz;
    cpuid(CPUID_TSC_LEAF, 0, &denominator, &numerator, &crystal_khz, &d);
    if (!denominator || !numerator) {
        LOG_TSC("TSC/Crystal ratio cannot be calculated.\n");
        return 0;
    }

    uint32_t crystal_hz;
    if (!crystal_khz) {
        LOG_TSC("CPU does not report Crystal frequency, deriving...\n");

        /* From [2]: "Some Intel SoCs like Skylake and Kabylake don't report the crystal
         * clock, but we can easily calculate it to a high degree of accuracy
         * by considering the crystal ratio and the CPU speed." */
        if (max_basic_leaf < CPUID_PROC_FREQ_LEAF) {
            return 0;
        }

        uint32_t proc_base_mhz;
        cpuid(CPUID_PROC_FREQ_LEAF, 0, &proc_base_mhz, &b, &c, &d);
        crystal_hz = proc_base_mhz * 1000 * 1000 * (denominator / (double)numerator);

        LOG_TSC("Processor base speed is %u MHz\n", proc_base_mhz);
        LOG_TSC("Crystal clock is %u Hz\n", crystal_hz);
    } else {
        crystal_hz = crystal_khz * 1000;
    }

    /* From [1]:
     * EAX: denominator of the TSC/”core crystal clock” ratio.
     * EBX: numerator of the TSC/”core crystal clock” ratio.
     * ECX: nominal frequency of the core crystal clock in Hz.
     * So “TSC frequency” = “core crystal clock frequency” * EBX/EAX.
     */

    uint64_t tsc_hz = (uint64_t)crystal_hz * (numerator / (double)denominator);
    LOG_TSC("detected TSC frequency is %u * (%u / %u) = %lu Hz\n", crystal_hz, numerator, denominator, tsc_hz);
    return tsc_hz;
}

static uint64_t measure_tsc_frequency(microkit_channel timer_ch)
{
    /* Measure TSC frequency */
    uint64_t unused, tsc_pre, tsc_post;
    // @billn hack
    tsc_pre = rdtsc();
    sddf_timer_set_timeout(timer_ch, NS_IN_S);
    seL4_Wait(BASE_ENDPOINT_CAP + timer_ch, &unused);
    tsc_post = rdtsc();

    uint64_t tsc_hz = tsc_post - tsc_pre;
    LOG_TSC("measured TSC frequency %lu Hz\n", tsc_hz);
    return tsc_hz;
}

uint64_t get_host_tsc_hz(microkit_channel timer_ch)
{
    uint64_t tsc_hz = 0;
    if (!is_intel_cpu()) {
        LOG_TSC("Not an Intel CPU.\n");
        return 0;
    } else {
        tsc_hz = get_tsc_frequency();
        if (!tsc_hz) {
            LOG_TSC("cannot detect TSC frequency via cpuid, manually measuring.\n");
        }
    }

    if (!tsc_hz) {
        /* Likely running on QEMU under nested virtualisation. QEMU doesn't enumerate TSC cpuid leafs */
        tsc_hz = measure_tsc_frequency(timer_ch);
    }

    return tsc_hz;
}