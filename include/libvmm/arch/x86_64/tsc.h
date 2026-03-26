/*
 * Copyright 2026, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>

typedef struct x86_host_tsc {
    bool valid;
    bool invariant;
    uint64_t freq_hz;
} x86_host_tsc_t;

x86_host_tsc_t get_host_tsc(microkit_channel timer_ch);