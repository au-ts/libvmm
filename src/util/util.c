/*
 * Copyright 2026, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sddf/util/util.h>
#include <libvmm/util/util.h>

bool check_baseline_bits(uint64_t baseline, uint64_t actual)
{
    return (actual & baseline) == baseline;
}

void print_missing_baseline_bits(uint64_t baseline, uint64_t actual)
{
    for (int i = 0; i < 64; i++) {
        if ((baseline & BIT(i)) && !(actual & BIT(i))) {
            printf("missing bit %d\n", i);
        }
    }
}
