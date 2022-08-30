/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include <stdbool.h>
#include "psci.h"
#include "smc.h"
#include "fault.h"
#include "util.h"

bool handle_psci(uint64_t vcpu_id, seL4_UserContext *regs, uint64_t fn_number)
{
    printf("VMM|INFO: handling PSCI\n");
    int err;
    // @ivanv: write a note about what convention we assume, should we be checking
    // the convention?
    switch (fn_number) {
        case PSCI_VERSION:
            // @ivanv: where does this value come from?
            smc_set_return_value(regs, 0x00010000); /* version 1 */
            break;
        case PSCI_CPU_ON: {
            uintptr_t target_cpu = smc_get_arg(regs, 1);
            // Right now we only have one vCPU and so any fault for a target vCPU
            // that isn't the one that's already on we consider a failure.
            // TODO(ivanv): adapt for starting other vCPUs
            if (target_cpu == VCPU_ID) {
                smc_set_return_value(regs, PSCI_ALREADY_ON);
            } else {
                // TODO(ivanv): is this really an internal failure? This seems like
                // the fault of the guest to me, not of the VMM.
                smc_set_return_value(regs, PSCI_INTERNAL_FAILURE);
            }
            break;
        }
        case PSCI_MIGRATE_INFO_TYPE:
            /* trusted OS does not require migration */
            smc_set_return_value(regs, 2);
            break;
        case PSCI_FEATURES:
            // @ivanv: a comment like the one below shouldn't be in the code base
            /* TODO Not sure if required */
            smc_set_return_value(regs, PSCI_NOT_SUPPORTED);
            break;
        case PSCI_SYSTEM_RESET:
            smc_set_return_value(regs, PSCI_SUCCESS);
            break;
        default:
            printf("VMM|ERROR: Unhandled PSCI function ID 0x%lx\n", fn_number);
            return false;
    }

    err = seL4_TCB_WriteRegisters(BASE_VM_TCB_CAP + VM_ID, false, 0, SEL4_USER_CONTEXT_SIZE, regs);
    printf("err seL4_TCB_WriteRegisters: 0x%lx\n", err);
    // halt(!err);
    advance_fault();

    return true;
}
