/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <sddf/util/util.h>
#include <libvmm/util/util.h>

bool handle_cr_access(seL4_VCPUContext *vctx, seL4_Word qualification);