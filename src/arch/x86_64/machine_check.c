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

/* Virtualisation of the x86 MCA and MCE */

/* Documents referenced:
 * 1. Intel® 64 and IA-32 Architectures Software Developer’s Manual
 *    Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *    Order Number: 325462-091US March 2026
 */

/* We present the guest with reporting banks, all of which are permanently
 * clean.
 */

/* "Table 2-2. IA-32 Architectural MSRs " */
#define IA32_MCG_CAP (0x179)
#define IA32_MCG_STATUS (0x17a)
#define IA32_MCG_CTL (0x17b)
#define IA32_MC0_CTL (0x400)
#define IA32_MCi_CTL(i) (IA32_MC0_CTL + 4 * (i) + 0)
#define IA32_MCi_STATUS(i) (IA32_MC0_CTL + 4 * (i) + 1)
#define IA32_MCi_ADDR(i) (IA32_MC0_CTL + 4 * (i) + 2)
#define IA32_MCi_MISC(i) (IA32_MC0_CTL + 4 * (i) + 3)

typedef enum { CTL = 0, STATUS, ADDR, MISC } mca_bank_reg_t;

/* IA32_MCG_CAP bits. We report only MCG_CTL_P "control MSR present".
 * See "18.3.1.1 IA32_MCG_CAP MSR" */
#define MCG_CAP_COUNT_MASK (0xffULL)
#define MCG_CAP_CTL_P (1ULL << 8)
#define MCA_BANK_COUNT 4 /* Chosen arbitrarily, Linux and Windows seems to be ok with it. */
#define IA32_MCG_CAP_VALUE ((MCA_BANK_COUNT & MCG_CAP_COUNT_MASK) | MCG_CAP_CTL_P)

static bool mca_decode_bank_msr(uint64_t msr, mca_bank_reg_t *reg)
{
    if (msr < IA32_MCi_CTL(0) || msr > IA32_MCi_MISC(MCA_BANK_COUNT - 1)) {
        return false;
    }

    *reg = (mca_bank_reg_t)((msr - IA32_MC0_CTL) % 4);
    return true;
}

bool msr_is_machine_check(uint64_t msr, bool is_read)
{
    unsigned int reg;

    if (is_read) {
        switch (msr) {
        case IA32_MCG_CAP:
        case IA32_MCG_STATUS:
        case IA32_MCG_CTL:
            return true;
        default:
            return mca_decode_bank_msr(msr, &reg);
        }
    } else {
        switch (msr) {
        /* IA32_MCG_CAP is read-only, but we still claim writes to it so
         * that we inject #GP ourselves rather than letting the access
         * reach hardware. */
        case IA32_MCG_CAP:
        case IA32_MCG_STATUS:
        case IA32_MCG_CTL:
            return true;
        default:
            return mca_decode_bank_msr(msr, &reg);
        }
    }
}

bool handle_machine_check_msr_read(uint64_t msr, uint64_t *result)
{
    unsigned int reg;

    switch (msr) {
    case IA32_MCG_CAP:
        /* "The number of banks that needs to be tracked is
         *  determined by IA32_MCG_CAP[7:0]" */
        *result = IA32_MCG_CAP_VALUE;
        break;
    case IA32_MCG_STATUS:
        /* No machine check is in progress. */
        *result = 0;
        break;
    case IA32_MCG_CTL:
        /* Reporting is enabled for every bank. */
        *result = ~0ULL;
        break;
    default:
        if (!mca_decode_bank_msr(msr, &reg)) {
            LOG_VMM_ERR("invalid MSR 0x%lx\n", msr);
            return false;
        }

        switch (reg) {
        case CTL:
            /* Every error type in the bank is enabled for reporting. */
            *result = ~0ULL;
            break;
        case STATUS:
            /* VAL is clear, so the guest sees no logged error and will not
             * look at ADDR or MISC. */
            *result = 0;
            break;
        case ADDR:
        case MISC:
            *result = 0;
            break;
        default:
            LOG_VMM_ERR("invalid MSR 0x%lx\n", msr);
            return false;
        }
        break;
    }

    return true;
}

bool handle_machine_check_msr_write(uint64_t msr, uint64_t value)
{
    unsigned int reg;

    switch (msr) {
    case IA32_MCG_CAP:
        LOG_VMM_ERR("guest wrote read-only MSR IA32_MCG_CAP\n");
        return false;
    case IA32_MCG_STATUS:
        if (value != 0) {
            LOG_VMM_ERR("guest wrote nonzero 0x%lx to IA32_MCG_STATUS\n", value);
            return false;
        }
        break;
    case IA32_MCG_CTL:
        /* The guest writes all-ones at init to enable reporting. We are
         * already reporting nothing, so discard the value. */
        break;
    default:
        if (!mca_decode_bank_msr(msr, &reg)) {
            LOG_VMM_ERR("invalid MSR 0x%lx\n", msr);
            return false;
        }

        switch (reg) {
        case CTL:
            /* Enabling or disabling reporting for a bank that can never
             * report anything is a no-op. */
            break;
        case STATUS:
            if (value != 0) {
                LOG_VMM_ERR("guest wrote nonzero 0x%lx to MCi_STATUS MSR 0x%lx\n", value, msr);
                return false;
            }
            break;
        case ADDR:
        case MISC:
            break;
        default:
            LOG_VMM_ERR("invalid MSR 0x%lx\n", msr);
            return false;
        }
        break;
    }

    return true;
}