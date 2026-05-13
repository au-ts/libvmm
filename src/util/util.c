/*
 * Copyright 2026, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/util/util.h>

bool ranges_overlap(uint64_t left_start, uint64_t left_end, uint64_t right_start, uint64_t right_end)
{
    return !(left_end <= right_start || right_end <= left_start);
}