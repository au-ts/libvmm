/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <microkit.h>

#define MTRR_MAX_VARIABLES 8

struct mtrr_state {
    uint64_t mtrr_def_type;
    uint64_t fixed_64k;    /* 0x250 */
    uint64_t fixed_16k[2]; /* 0x258, 0x259 */
    uint64_t fixed_4k[8];  /* 0x268 - 0x26F */

    struct {
        uint64_t base;
        uint64_t mask;
    } variable[MTRR_MAX_VARIABLES];
};

bool initialise_msrs(bool bsp);
bool emulate_rdmsr(seL4_VCPUContext *vctx);
bool emulate_wrmsr(seL4_VCPUContext *vctx);
