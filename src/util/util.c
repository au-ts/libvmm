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
