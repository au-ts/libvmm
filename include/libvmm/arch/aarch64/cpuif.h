/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>

typedef enum sysreg_perm {
    SYSREG_READ = 0,
    SYSREG_WRITE = 1,
} sysreg_perm_t;

typedef bool (*sysreg_read_exception_handler_t)(size_t vcpu_id);
typedef bool (*sysreg_write_exception_handler_t)(size_t vcpu_id, uint64_t data);

/* Details of system registers we currently emulate. */
typedef struct arm_sysreg_info {
    /* For debugging only. */
    char *name;

    /* Location of register.  */
    uint8_t crn;
    uint8_t crm;
    uint8_t opc0;
    uint8_t opc1;
    uint8_t opc2;

    /* Permission */
    sysreg_perm_t perm;

    /* Access functions */
    sysreg_read_exception_handler_t read_fn;
    sysreg_write_exception_handler_t write_fn;
} arm_sysreg_info_t;

const arm_sysreg_info_t cpuif_reginfo[] = {
    {
        .name = "ICC_SGI1R_EL1",
        .opc0 = 3, .opc1 = 0, .crn = 12, .crm = 11, .opc2 = 5,
        .perm = SYSREG_WRITE,
        .read_fn = NULL,
        .write_fn = NULL, // @billn fix
    }
};

/* Handles exception from MSR, MRS, or System instruction execution in AArch64 state (EC class 0x18). */
bool handle_sysreg_64_fault(size_t vcpu_id, uint32_t hsr, seL4_UserContext *regs);
