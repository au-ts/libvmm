/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/aarch64/vgic/vgic.h>
#include <libvmm/arch/aarch64/cpuif.h>
#include <libvmm/arch/aarch64/fault.h>
#include <libvmm/arch/aarch64/vgic/vgic_v3_cpuif.h>

#define ESR_EL2_IL_BIT 25

/* The ISS encoding was taken from "Arm A-profile Architecture Registers" DDI0601,
   section "ESR_EL2, Exception Syndrome Register (EL2)",
   heading "ISS encoding for an exception from MSR, MRS, or System instruction execution in AArch64 state" */
#define ISS_MASK_FROM_HSR      0xffffff
#define ISS_SYSREG_IS_READ_BIT 0
#define ISS_SYSREG_CRM_SHIFT   1
#define ISS_SYSREG_CRM_MASK    0xf
#define ISS_SYSREG_RT_SHIFT    5
#define ISS_SYSREG_RT_MASK     0x1f
#define ISS_SYSREG_CRN_SHIFT   10
#define ISS_SYSREG_CRN_MASK    0xf
#define ISS_SYSREG_OP1_SHIFT   14
#define ISS_SYSREG_OP1_MASK    0x7
#define ISS_SYSREG_OP2_SHIFT   17
#define ISS_SYSREG_OP2_MASK    0x7
#define ISS_SYSREG_OP0_SHIFT   20
#define ISS_SYSREG_OP0_MASK    0x3

/* Data structure of system registers we currently emulate. */
typedef struct aarch64_sysreg_info {
    /* For debugging only. */
    char *name;

    /* Location of register. */
    uint8_t crn;
    uint8_t crm;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;

    /* Access functions for both direction. NULL if access will be denied. */
    sysreg_read_exception_handler_t read_fn;
    sysreg_write_exception_handler_t write_fn;
} aarch64_sysreg_info_t;

static const aarch64_sysreg_info_t cpuif_reginfo[] = {
#if defined(GIC_V3)
    /* The following registers values were taken from:
       "Arm Generic Interrupt Controller Architecture Specification GIC architecture version 3 and version 4".
       Document IHI0069H.b ID041224. */
    {
        /* Page 12-276 */
        .name = "ICC_SGI1R_EL1",
        .opc0 = 3,
        .opc1 = 0,
        .crn = 12,
        .crm = 11,
        .opc2 = 5,
        .read_fn = NULL,
        .write_fn = icc_sgi1r_el1_write,
    }
#endif
};

static int sysreg_fault_get_rt(uint64_t hsr)
{
    /* Make sure the instruction length bit == 1 for 32-bits instructions. */
    if (BIT_LOW(ESR_EL2_IL_BIT)) {
        return (hsr >> ISS_SYSREG_RT_SHIFT) & ISS_SYSREG_RT_MASK;
    } else {
        printf("sysreg_fault_get_rt() for 16-bits instructions not implemented.\n");
        assert(false);
    }
}

/* We can't use `fault_get_data()` because the HSR register encoding is different in this scenario. */
static uint64_t sysreg_fault_get_data(seL4_UserContext *regs, uint64_t hsr)
{
    /* Get register opearand */
    int rt = sysreg_fault_get_rt(hsr);
    uint64_t data = *decode_rt(rt, regs);
    return data;
}

bool handle_sysreg_64_fault(size_t vcpu_id, uint64_t hsr, seL4_UserContext *regs)
{
    uint32_t iss = hsr & ISS_MASK_FROM_HSR;
    uint8_t op0 = (iss >> ISS_SYSREG_OP0_SHIFT) & ISS_SYSREG_OP0_MASK;
    uint8_t op2 = (iss >> ISS_SYSREG_OP2_SHIFT) & ISS_SYSREG_OP2_MASK;
    uint8_t op1 = (iss >> ISS_SYSREG_OP1_SHIFT) & ISS_SYSREG_OP1_MASK;
    uint8_t crn = (iss >> ISS_SYSREG_CRN_SHIFT) & ISS_SYSREG_CRN_MASK;
    uint8_t crm = (iss >> ISS_SYSREG_CRM_SHIFT) & ISS_SYSREG_CRM_MASK;
    bool is_read = (iss >> ISS_SYSREG_IS_READ_BIT) & 0x1;
    uint64_t data = sysreg_fault_get_data(regs, hsr);

    for (int i = 0; i < ARRAY_SIZE(cpuif_reginfo); i++) {
        if (cpuif_reginfo[i].opc0 == op0 && cpuif_reginfo[i].opc1 == op1 && cpuif_reginfo[i].crn == crn
            && cpuif_reginfo[i].crm == crm && cpuif_reginfo[i].opc2 == op2) {
            /* Check access rights */
            if (is_read) {
                assert(cpuif_reginfo[i].read_fn);
                return cpuif_reginfo[i].read_fn(vcpu_id, regs);
            } else {
                assert(cpuif_reginfo[i].write_fn);
                return cpuif_reginfo[i].write_fn(vcpu_id, regs, data);
            }
        }
    }

    LOG_VMM("trapped sysreg 64 exception with unimplemented instruction: op0 %u, op2 %u, op1 %u, crn %u, crm %u, is "
            "read %u\n",
            op0, op2, op1, crn, crm, is_read);
    return false;
}
