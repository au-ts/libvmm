/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>
#include "smc.h"
#include "psci.h"
#include "util.h"

// Values in this file are taken from:
// SMC CALLING CONVENTION
// System Software on ARM (R) Platforms
// Issue B

// @ivanv: are these #defines used anywhere?
#define SMC_CALLING_CONVENTION (1 << 31)
#define SMC_CALLING_CONVENTION_32 0
#define SMC_CALLING_CONVENTION_64 1
#define SMC_FAST_CALL (1 << 31)

#define SMC_SERVICE_CALL_MASK 0x3F
#define SMC_SERVICE_CALL_SHIFT 24

#define SMC_FUNC_ID_MASK 0xFFFF

/* SMC and HVC function identifiers */
typedef enum {
    SMC_CALL_ARM_ARCH = 0,
    SMC_CALL_CPU_SERVICE = 1,
    SMC_CALL_SIP_SERVICE = 2,
    SMC_CALL_OEM_SERVICE = 3,
    SMC_CALL_STD_SERVICE = 4,
    SMC_CALL_STD_HYP_SERVICE = 5,
    SMC_CALL_VENDOR_HYP_SERVICE = 6,
    SMC_CALL_TRUSTED_APP = 48,
    SMC_CALL_TRUSTED_OS = 50,
    SMC_CALL_RESERVED = 64,
} smc_call_id_t;

static smc_call_id_t smc_get_call(uintptr_t func_id)
{
    uint64_t service = ((func_id >> SMC_SERVICE_CALL_SHIFT) & SMC_SERVICE_CALL_MASK);
    halt(service >= 0 && service <= 0xFFFF);

    if (service <= SMC_CALL_VENDOR_HYP_SERVICE) {
        return service;
    } else if (service < SMC_CALL_TRUSTED_APP) {
        return SMC_CALL_RESERVED;
    } else if (service < SMC_CALL_TRUSTED_OS) {
        return SMC_CALL_TRUSTED_APP;
    } else if (service < SMC_CALL_RESERVED) {
        return SMC_CALL_TRUSTED_OS;
    } else {
        return SMC_CALL_RESERVED;
    }
}

// @ivanv: remove?
// static bool smc_call_is_32(uintptr_t func_id)
// {
//     return !!(func_id & SMC_CALLING_CONVENTION) == SMC_CALLING_CONVENTION_32;
// }

static bool smc_call_is_atomic(uintptr_t func_id)
{
    return !!(func_id & SMC_FAST_CALL);
}

static uintptr_t smc_get_function_number(uintptr_t func_id)
{
    return (func_id & SMC_FUNC_ID_MASK);
}

static uint64_t smc_get_function_id(seL4_UserContext *u)
{
    return u->x0;
}

void smc_set_return_value(seL4_UserContext *u, uint64_t val)
{
    u->x0 = val;
}

uint64_t smc_get_arg(seL4_UserContext *u, uint64_t arg)
{
    switch (arg) {
        case 1: return u->x1;
        case 2: return u->x2;
        case 3: return u->x3;
        case 4: return u->x4;
        case 5: return u->x5;
        case 6: return u->x6;
        default:
            printf("VMM|ERROR: trying to get SMC arg: 0x%lx, SMC only has 6 argument registers\n", arg);
            // @ivanv: come back to this
            return 0;
    }
}

static void smc_set_arg(seL4_UserContext *u, uint64_t arg, uint64_t val)
{
    switch (arg) {
        case 1: u->x1 = val; break;
        case 2: u->x2 = val; break;
        case 3: u->x3 = val; break;
        case 4: u->x4 = val; break;
        case 5: u->x5 = val; break;
        case 6: u->x6 = val; break;
        default:
            printf("VMM|ERROR: trying to set SMC arg: 0x%lx, with val: 0x%lx, SMC only has 6 argument registers\n", arg, val);
    }
}

bool handle_smc(uint32_t hsr)
{
    printf("VMM|INFO: handling SMC event\n");
    // @ivanv: An optimisation to be made is to store the TCB registers so we don't
    // end up reading them multiple times
    seL4_UserContext regs;
    int err = seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + VM_ID, false, 0, SEL4_USER_CONTEXT_SIZE, &regs);
    printf("err: 0x%lx\n", err);
    // halt(err != seL4_NoError);

    uint64_t id = smc_get_function_id(&regs);
    uint64_t fn_number = smc_get_function_number(id);
    smc_call_id_t service = smc_get_call(id);

    switch (service) {
        case SMC_CALL_ARM_ARCH:
            printf("VMM|ERROR: Unhandled SMC: ARM architecture call %lu\n", fn_number);
            break;
        case SMC_CALL_CPU_SERVICE:
            printf("VMM|ERROR: Unhandled SMC: CPU service call %lu\n", fn_number);
            break;
        case SMC_CALL_SIP_SERVICE:
            printf("VMM|ERROR: Unhandled SMC: SiP service call %lu\n", fn_number);
            break;
        case SMC_CALL_OEM_SERVICE:
            printf("VMM|ERROR: Unhandled SMC: OEM service call %lu\n", fn_number);
            break;
        case SMC_CALL_STD_SERVICE:
            if (fn_number < PSCI_MAX) {
                printf("VMM|INFO: handling PSCI\n");
                return handle_psci(VCPU_ID, &regs, fn_number, hsr);
            }
            printf("VMM|ERROR: Unhandled SMC: standard service call %lu\n", fn_number);
            break;
        case SMC_CALL_STD_HYP_SERVICE:
            printf("VMM|ERROR: Unhandled SMC: standard hyp service call %lu\n", fn_number);
            break;
        case SMC_CALL_VENDOR_HYP_SERVICE:
            printf("VMM|ERROR: Unhandled SMC: vendor hyp service call %lu\n", fn_number);
            break;
        case SMC_CALL_TRUSTED_APP:
            printf("VMM|ERROR: Unhandled SMC: trusted app call %lu\n", fn_number);
            break;
        case SMC_CALL_TRUSTED_OS:
            printf("VMM|ERROR: Unhandled SMC: trusted OS call %lu\n", fn_number);
            break;
        default:
            printf("VMM|ERROR: Unhandled SMC: unknown value service: %lu fn_number: %lu\n",
                    (unsigned long) service, fn_number);
            break;
    }

    return false;
}
