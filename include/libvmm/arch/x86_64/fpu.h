/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>
#include <microkit.h>

/* Higher level fault handling logic must raise general protection exception #GP(0)
 * if the return value is false */
bool emulate_xsetbv(seL4_VCPUContext *vctx);