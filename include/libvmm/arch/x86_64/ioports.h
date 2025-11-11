/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>

bool emulate_ioports(seL4_VCPUContext *vctx, uint64_t f_qualification);