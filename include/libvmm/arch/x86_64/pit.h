/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>

// Programmable Interval Timer
bool emulate_pit(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read);
void pit_handle_timer_ntfn(void);