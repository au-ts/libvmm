/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Document referenced
 * 1. https://www.felixcloutier.com/x86/xsetbv
 * 2. seL4 source: src/arch/x86/machine/fpu.c
 */

#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>

bool emulate_xsetbv(seL4_VCPUContext *vctx)
{
    /* [1] "Writes the contents of registers EDX:EAX into the 64-bit
     * extended control register (XCR) specified in the ECX register." */

    /* "Currently, only XCR0 is supported. Thus, all other values of ECX are reserved and will cause a #GP(0)." */
    if (vctx->ecx != 0) {
        return false;
    }

    /* seL4 will set the host XCR0 to CONFIG_XSAVE_FEATURE_SET, so we
     * just need to make sure that the guest is setting the same value.
     * See [2] Arch_initFpu() */
    uint64_t value = (uint64_t)((vctx->edx & 0xffffffff) << 32) | (uint64_t)(vctx->eax & 0xffffffff);
    if (value != CONFIG_XSAVE_FEATURE_SET) {
        return false;
    }

    return true;
}