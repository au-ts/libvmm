/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>
#include <libvmm/util/util.h>

// intel manual
// [1] Table 28-5. Exit Qualification for I/O Instructions

// [1]
typedef enum ioport_access_width_qualification {
    IOPORT_BYTE_ACCESS_QUAL = 0,
    IOPORT_WORD_ACCESS_QUAL = 1, // 2-byte
    IOPORT_DWORD_ACCESS_QUAL = 3, // 4-byte
} ioport_access_width_t;

int ioports_access_width_to_bytes(ioport_access_width_t access_width);

void emulate_ioport_string(seL4_VCPUContext *vctx, char *data, size_t len, ioport_access_width_t access_width);
bool emulate_ioports(seL4_VCPUContext *vctx, uint64_t f_qualification);
