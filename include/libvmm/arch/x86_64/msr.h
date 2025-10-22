/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>

bool emulate_rdmsr(seL4_VCPUContext *vctx);
bool emulate_wrmsr(seL4_VCPUContext *vctx);
