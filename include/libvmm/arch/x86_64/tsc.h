/*
 * Copyright 2026, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>

/* Determines the host CPU's TSC frequency. First attempt to
   calculate it from values in the appropriate CPUID leafs. If
   they aren't enumerated, which happens under QEMU. Measure
   it with the timer. */
uint64_t get_host_tsc_hz(microkit_channel timer_ch);