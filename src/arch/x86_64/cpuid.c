/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sddf/util/util.h>
#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/cpuid.h>

static inline void cpuid(uint32_t leaf, uint32_t subleaf,
                         uint32_t *a, uint32_t *b,
                         uint32_t *c, uint32_t *d) {
    __asm__ __volatile__(
        "cpuid"
        : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d)
        : "a"(leaf), "c"(subleaf)
    );
}

bool emulate_cpuid(seL4_VCPUContext *vctx) {
    // @billn todo revisit likely need to turn on some important features.
    // 3-218 Vol. 2A

    if (vctx->eax == 0) {
        // 3-240 Vol. 2A
        // GenuineIntel
        vctx->eax = 0x1; // ???
        vctx->ebx = 0x756e6547;
        vctx->ecx = 0x49656e69;
        vctx->edx = 0x6c65746e;
    } else if (vctx->eax == 1) {
        // Encoding from:
        // https://en.wikipedia.org/wiki/CPUID
        // "EAX=1: Processor Info and Feature Bits"

        // Values from:
        // https://en.wikichip.org/wiki/intel/cpuid
        // Using value for "Haswell (Client)" Microarch and "HSW-U" Core

        // OEM processor: bit 12 and 13 zero
        uint32_t model_id = 0x5 << 4;
        uint32_t ext_model_id = 0x4 << 16;
        // Pentium and Intel Core family
        uint32_t family_id = 0x6 << 8;

        vctx->eax = model_id | ext_model_id | family_id;

    } else if (vctx->eax == 0x80000000) {
        vctx->eax = 0;
    } else if (vctx->eax == 0x80000001) {
        vctx->eax = 0;
    } else {
        LOG_VMM_ERR("invalid cpuid eax value: 0x%x\n", vctx->eax);
        return false;
    }

    return true;
}