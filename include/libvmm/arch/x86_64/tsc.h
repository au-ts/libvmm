/*
 * Copyright 2026, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <microkit.h>

static inline uint64_t tsc_hz_from_bootinfo(seL4_BootInfoHeader *tsc_bootinfo_mr)
{
    if (!tsc_bootinfo_mr || tsc_bootinfo_mr->id != SEL4_BOOTINFO_HEADER_X86_TSC_FREQ
        || tsc_bootinfo_mr->len < sizeof(*tsc_bootinfo_mr) + sizeof(uint32_t)) {
        return 0;
    }
    uint32_t tsc_mhz = *(uint32_t *)(((uint8_t *)tsc_bootinfo_mr) + sizeof(seL4_BootInfoHeader));
    return tsc_mhz * 1000000ull;
}
