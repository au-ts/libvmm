/*
 * Copyright 2026, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sddf/util/util.h>
#include <sddf/util/udivmodti4.h>
#include <libvmm/util/util.h>

#define LOW_128B_WORD(x) (x & 0xffffffffffffffff)
#define HIGH_128B_WORD(x) (x >> 64)

bool check_baseline_bits(uint64_t baseline, uint64_t actual)
{
    return (actual & baseline) == baseline;
}

void print_missing_baseline_bits(uint64_t baseline, uint64_t actual)
{
    for (int i = 0; i < 64; i++) {
        if ((baseline & BIT(i)) && !(actual & BIT(i))) {
            LOG_VMM_ERR("missing bit %d\n", i);
        }
    }
}

bool ranges_overlap(uint64_t left_start, uint64_t left_end, uint64_t right_start, uint64_t right_end)
{
    return !(left_end <= right_start || right_end <= left_start);
}

uint64_t convert_ticks_by_frequency(uint64_t ticks, uint64_t in_freq, uint64_t out_freq)
{
    if (in_freq == 0)
        return 0;
    __uint128_t intermediate = (__uint128_t)ticks * out_freq;
    uint64_t rem;
    return udiv128by64to64(HIGH_128B_WORD(intermediate), LOW_128B_WORD(intermediate), in_freq, &rem);
}

#if defined(CONFIG_ARCH_X86_64)
/*
 * CPUID bits for detecting the kernel is running as a guest.
 *
 * These bits correspond to the strings "KVMKVMKVM" for KVM and "TCGTCGTCGTCG"
 * for QEMU's Tiny Code Generator (TCG).
 *
 * https://docs.kernel.org/virt/kvm/x86/cpuid.html.
 */
#define KVM_CPUID_SIGNATURE 0x40000000

#define CPUID_TCG_EBX 0x54474354
#define CPUID_TCG_ECX 0x43544743
#define CPUID_TCG_EDX 0x47435447
#define CPUID_KVM_EBX 0x4b4d564b
#define CPUID_KVM_ECX 0x564b4d56
#define CPUID_KVM_EDX 0x4d

bool hypervisor_present(void)
{
    uint32_t a, b, c, d;
    cpuid(KVM_CPUID_SIGNATURE, 0, &a, &b, &c, &d);

    if ((b == CPUID_KVM_EBX && c == CPUID_KVM_ECX && d == CPUID_KVM_EDX)
        || (b == CPUID_TCG_EBX && c == CPUID_TCG_ECX && d == CPUID_TCG_EDX)) {
        return true;
    }
    return false;
}

#endif