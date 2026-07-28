/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <sddf/util/util.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/msr.h>

/* Virtualisation of the x86 Memory Type Range Registers */

/* Documents referenced:
 * 1. Intel® 64 and IA-32 Architectures Software Developer’s Manual
 *    Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *    Order Number: 325462-080US June 2023
 */

#define IA32_MTRRCAP (0xfe)
#define IA32_MTRR_DEF_TYPE (0x2ff)
#define IA32_MTRR_PHYSBASE0 (0x200)
#define IA32_MTRR_PHYSMASK0 (0x201)
#define IA32_MTRR_PHYSBASE1 (0x202)
#define IA32_MTRR_PHYSMASK1 (0x203)
#define IA32_MTRR_PHYSBASE2 (0x204)
#define IA32_MTRR_PHYSMASK2 (0x205)
#define IA32_MTRR_PHYSBASE3 (0x206)
#define IA32_MTRR_PHYSMASK3 (0x207)
#define IA32_MTRR_PHYSBASE4 (0x208)
#define IA32_MTRR_PHYSMASK4 (0x209)
#define IA32_MTRR_PHYSBASE5 (0x20A)
#define IA32_MTRR_PHYSMASK5 (0x20B)
#define IA32_MTRR_PHYSBASE6 (0x20C)
#define IA32_MTRR_PHYSMASK6 (0x20D)
#define IA32_MTRR_PHYSBASE7 (0x20E)
#define IA32_MTRR_PHYSMASK7 (0x20F)
#define IA32_MTRR_FIX64K_00000 (0x250)
#define IA32_MTRR_FIX16K_80000 (0x258)
#define IA32_MTRR_FIX16K_A0000 (0x259)
#define IA32_MTRR_FIX4K_C0000 (0x268)
#define IA32_MTRR_FIX4K_C8000 (0x269)
#define IA32_MTRR_FIX4K_D0000 (0x26A)
#define IA32_MTRR_FIX4K_D8000 (0x26B)
#define IA32_MTRR_FIX4K_E0000 (0x26C)
#define IA32_MTRR_FIX4K_E8000 (0x26D)
#define IA32_MTRR_FIX4K_F0000 (0x26E)
#define IA32_MTRR_FIX4K_F8000 (0x26F)

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

static struct mtrr_state mtrr_state;

bool initialise_mtrr(void)
{
    /* See "IA32_MTRR_DEF_TYPE MSR"
     * Enable MTRRs */
    mtrr_state.mtrr_def_type = BIT(11);
    return true;
}

bool msr_is_mtrr(uint64_t msr, bool is_read)
{
    if (is_read) {
        switch (msr) {
        case IA32_MTRRCAP:
        case IA32_MTRR_DEF_TYPE:
        case IA32_MTRR_PHYSBASE0 ... IA32_MTRR_PHYSMASK7:
        case IA32_MTRR_FIX64K_00000:
        case IA32_MTRR_FIX16K_80000 ... IA32_MTRR_FIX16K_A0000:
        case IA32_MTRR_FIX4K_C0000 ... IA32_MTRR_FIX4K_F8000:
            return true;
        default:
            return false;
        }
    } else {
        switch (msr) {
        case IA32_MTRR_DEF_TYPE:
        case IA32_MTRR_PHYSBASE0 ... IA32_MTRR_PHYSMASK7:
        case IA32_MTRR_FIX64K_00000:
        case IA32_MTRR_FIX16K_80000 ... IA32_MTRR_FIX16K_A0000:
        case IA32_MTRR_FIX4K_C0000 ... IA32_MTRR_FIX4K_F8000:
            return true;
        default:
            return false;
        }
    }
}

bool handle_mtrr_msr_read(uint64_t msr, uint64_t *result)
{
    switch (msr) {
    case IA32_MTRRCAP:
        /* "Table 2-2. IA-32 Architectural MSRs (Contd.)" "IA32_MTRRCAP (MTRRcap)"
           "Fixed range MTRRs are supported when set" + WC supported. */
        *result = MTRR_MAX_VARIABLES | BIT(8) | BIT(10);
        break;
    case IA32_MTRR_DEF_TYPE:
        *result = mtrr_state.mtrr_def_type;
        break;
    case IA32_MTRR_PHYSBASE0 ... IA32_MTRR_PHYSMASK7: {
        int index = (msr - IA32_MTRR_PHYSBASE0) / 2;
        int is_mask = msr % 2;
        if (is_mask) {
            *result = mtrr_state.variable[index].mask;
        } else {
            *result = mtrr_state.variable[index].base;
        }
        break;
    }
    case IA32_MTRR_FIX64K_00000:
        *result = mtrr_state.fixed_64k;
        break;
    case IA32_MTRR_FIX16K_80000 ... IA32_MTRR_FIX16K_A0000: {
        int index = msr - IA32_MTRR_FIX16K_80000;
        *result = mtrr_state.fixed_16k[index];
        break;
    }
    case IA32_MTRR_FIX4K_C0000 ... IA32_MTRR_FIX4K_F8000: {
        int index = msr - IA32_MTRR_FIX4K_C0000;
        *result = mtrr_state.fixed_4k[index];
        break;
    }
    default:
        LOG_VMM_ERR("invalid MSR 0x%lx\n", msr);
        return false;
    }
    return true;
}

bool handle_mtrr_msr_write(uint64_t msr, uint64_t value)
{
    switch (msr) {
    case IA32_MTRR_DEF_TYPE:
        mtrr_state.mtrr_def_type = value;
        break;
    case IA32_MTRR_PHYSBASE0 ... IA32_MTRR_PHYSMASK7: {
        int index = (msr - IA32_MTRR_PHYSBASE0) / 2;
        int is_mask = msr % 2;
        if (is_mask) {
            mtrr_state.variable[index].mask = value;
        } else {
            mtrr_state.variable[index].base = value;
        }
        break;
    }
    case IA32_MTRR_FIX64K_00000:
        mtrr_state.fixed_64k = value;
        break;
    case IA32_MTRR_FIX16K_80000 ... IA32_MTRR_FIX16K_A0000: {
        int index = msr - IA32_MTRR_FIX16K_80000;
        mtrr_state.fixed_16k[index] = value;
        break;
    }
    case IA32_MTRR_FIX4K_C0000 ... IA32_MTRR_FIX4K_F8000: {
        int index = msr - IA32_MTRR_FIX4K_C0000;
        mtrr_state.fixed_4k[index] = value;
        break;
    }
    default:
        LOG_VMM_ERR("invalid MSR 0x%lx\n", msr);
        return false;
    }
    return true;
}