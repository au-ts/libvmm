/*
 * Copyright 2026, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <string.h>
#include <stddef.h>
#include <stdbool.h>

#define CMOS_PORT_ADDR 0x70
#define CMOS_PORT_SIZE 0x2

bool cmos_fault_handle(size_t vcpu_id, uint16_t port_offset, size_t qualification, seL4_VCPUContext *vctx,
                       void *cookie);