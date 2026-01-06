/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/ioports.h>

bool emulate_cmos_access(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read, ioport_access_width_t access_width);