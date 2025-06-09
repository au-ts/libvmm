/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/aarch64/cpuif.h>
#include <libvmm/arch/aarch64/fault.h>
#include <libvmm/arch/aarch64/vgic/vgic_v3_cpuif.h>

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

    /* Access control function, returns true if the operation is permitted. */
    sysreg_access_fn_t access_fn;

    /* Access functions for both direction. */
    sysreg_read_exception_handler_t read_fn;
    sysreg_write_exception_handler_t write_fn;
} aarch64_sysreg_info_t;

const aarch64_sysreg_info_t cpuif_reginfo[] = {
    {
        .name = "ICC_SGI1R_EL1",
        .opc0 = 3, .opc1 = 0, .crn = 12, .crm = 11, .opc2 = 5,
        .access_fn = icc_sgi1r_el1_access,
        .read_fn = NULL,
        .write_fn = icc_sgi1r_el1_write,
    }
};

bool handle_sysreg_64_fault(size_t vcpu_id, uint32_t hsr, seL4_UserContext *regs) {
    // @billn this code is a bit stupid, todo fix
    uint32_t iss = hsr & 0xffffff;
    uint8_t op0 = (iss >> 20) & 0x3;
    uint8_t op2 = (iss >> 17) & 0x7;
    uint8_t op1 = (iss >> 14) & 0x7;
    uint8_t crn = (iss >> 10) & 0xf;
    uint8_t crm = (iss >> 1) & 0xf;
    bool is_read = iss & 0x1;

    // @billn hack, should be using fault get data! and check whether itis actually a write
    size_t reg_idx = (hsr >> 5) & 0x1f;
    size_t data = *decode_rt(reg_idx, regs);

    for (int i = 0; i < sizeof(cpuif_reginfo) / sizeof(cpuif_reginfo[0]); i++) {
        if (
            cpuif_reginfo[i].opc0 == op0 &&
            cpuif_reginfo[i].opc1 == op1 &&
            cpuif_reginfo[i].crn == crn &&
            cpuif_reginfo[i].crm == crm &&
            cpuif_reginfo[i].opc2 == op2
        ) {
            /* Check access rights */
            if (cpuif_reginfo[i].access_fn(vcpu_id, regs, is_read)) {
                /* Ok to access, perform fault emulation. */
                if (is_read) {
                    assert(cpuif_reginfo[i].read_fn);
                    return cpuif_reginfo[i].read_fn(vcpu_id, regs);
                } else {
                    assert(cpuif_reginfo[i].write_fn);
                    return cpuif_reginfo[i].write_fn(vcpu_id, regs, data);
                }
            }
        }
    }

    LOG_VMM("trapped sysreg 64 exception with unknown instruction: op0 %u, op2 %u, op1 %u, crn %u, crm %u, is read %u\n", op0, op2, op1, crn, crm, is_read);
    return false;
}
