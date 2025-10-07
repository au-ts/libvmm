/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <microkit.h>

#if defined(CONFIG_PLAT_ODROIDC4)
#define NUM_EXPECTED_PASS_THRU_IRQS 1
#else
#error Unsupported platform
#endif

#define NUM_EXPECTED_UIO_REGIONS 11