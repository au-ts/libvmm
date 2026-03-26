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

/* Documents referenced:
 * 1. Intel® 64 and IA-32 Architectures Software Developer’s Manual
 *    Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *    Order Number: 325462-080US June 2023
 */

static bool is_invariant_tsc(void)
{
    /* [1] page "Vol. 2A 3-245" */
    /* Check "Maximum Input Value for Extended Function CPUID Information." */
    uint32_t a, b, c, d;
    cpuid(0x80000000, 0, &a, &b, &c, &d);

    if (a < 0x80000007) {
        return false;
    }

    /* Then check whether Invariant TSC is true */
    cpuid(0x80000007, 0, &a, &b, &c, &d);
    return !!(d & BIT(8));
}

static uint64_t measure_tsc_frequency(microkit_channel timer_ch)
{
    /* Measure TSC frequency */
    // @billn the tsc frequency is already measured and reported by seL4
    // via bootinfo, we should have a mechanism to pass this to PDs in Microkit
    uint64_t unused, tsc_pre, tsc_post;
    // @billn hack
    tsc_pre = rdtsc();
    sddf_timer_set_timeout(timer_ch, NS_IN_S);
    seL4_Wait(BASE_ENDPOINT_CAP + timer_ch, &unused);
    tsc_post = rdtsc();

    uint64_t tsc_hz = tsc_post - tsc_pre;

    // Round down to nearest 100MHz to account for measurement
    // interference. Linux is extremely picky about accurate TSC.
    return (tsc_hz / 100000000ull) * 100000000ull;
}

x86_host_tsc_t get_host_tsc(microkit_channel timer_ch)
{
    x86_host_tsc_t tsc_metadata = {
        .valid = true,
        .invariant = false,
        .freq_hz = 0,
    };

    tsc_metadata.freq_hz = measure_tsc_frequency(timer_ch);
    if (tsc_metadata.freq_hz == 0) {
        tsc_metadata.valid = false;
    } else {
        LOG_VMM("TSC: %llu Hz\n", tsc_metadata.freq_hz);
    }

    tsc_metadata.invariant = is_invariant_tsc();

    return tsc_metadata;
}