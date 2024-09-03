/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/util/util.h>
#include <libvmm/arch/aarch64/smc.h>
#include <libvmm/arch/aarch64/psci.h>
#include <libvmm/arch/aarch64/fault.h>

// #define DEBUG_SMC

// Values in this file are taken from:
// SMC CALLING CONVENTION
// System Software on ARM (R) Platforms
// Issue B
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

static smc_call_id_t smc_get_call(size_t func_id)
{
    uint64_t service = ((func_id >> SMC_SERVICE_CALL_SHIFT) & SMC_SERVICE_CALL_MASK);
    assert(service >= 0 && service <= 0xFFFF);

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

static inline uint64_t smc_get_function_number(seL4_UserContext *regs)
{
    return regs->x0 & SMC_FUNC_ID_MASK;
}

inline void smc_set_return_value(seL4_UserContext *u, uint64_t val)
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
            LOG_VMM_ERR("trying to get SMC arg: 0x%lx, SMC only has 6 argument registers\n", arg);
            // @ivanv: come back to this
            return 0;
    }
}

static void smc_set_arg(seL4_UserContext *u, size_t arg, size_t val)
{
    switch (arg) {
        case 1: u->x1 = val; break;
        case 2: u->x2 = val; break;
        case 3: u->x3 = val; break;
        case 4: u->x4 = val; break;
        case 5: u->x5 = val; break;
        case 6: u->x6 = val; break;
        default:
            LOG_VMM_ERR("trying to set SMC arg: 0x%lx, with val: 0x%lx, SMC only has 6 argument registers\n", arg, val);
    }
}

static void dump_smc_request(seL4_ARM_SMCContext *request) {
    LOG_VMM("SMC forward dump request:\n");
    LOG_VMM("   x0: 0x%lx\n", request->x0);
    LOG_VMM("   x1: 0x%lx\n", request->x1);
    LOG_VMM("   x2: 0x%lx\n", request->x2);
    LOG_VMM("   x3: 0x%lx\n", request->x3);
    LOG_VMM("   x4: 0x%lx\n", request->x4);
    LOG_VMM("   x5: 0x%lx\n", request->x5);
    LOG_VMM("   x6: 0x%lx\n", request->x6);
    LOG_VMM("   x7: 0x%lx\n", request->x7);
}

static void dump_smc_response(seL4_ARM_SMCContext *response) {
    LOG_VMM("SMC forward dump response:\n");
    LOG_VMM("   x0: 0x%lx\n", response->x0);
    LOG_VMM("   x1: 0x%lx\n", response->x1);
    LOG_VMM("   x2: 0x%lx\n", response->x2);
    LOG_VMM("   x3: 0x%lx\n", response->x3);
    LOG_VMM("   x4: 0x%lx\n", response->x4);
    LOG_VMM("   x5: 0x%lx\n", response->x5);
    LOG_VMM("   x6: 0x%lx\n", response->x6);
    LOG_VMM("   x7: 0x%lx\n", response->x7);
}

#if defined(CONFIG_ALLOW_SMC_CALLS)
bool smc_sip_forward(size_t vcpu_id, seL4_UserContext *regs, size_t fn_number)
{
    seL4_ARM_SMCContext request;
    seL4_ARM_SMCContext response;

    request.x0 = regs->x0; request.x1 = regs->x1;
    request.x2 = regs->x2; request.x3 = regs->x3;
    request.x4 = regs->x4; request.x5 = regs->x5;
    request.x6 = regs->x6; request.x7 = regs->x7;

#if defined(DEBUG_SMC)
    dump_smc_request(&request);
#endif

    microkit_arm_smc_call(&request, &response);

#if defined(DEBUG_SMC)
    dump_smc_response(&response);
#endif

    regs->x0 = response.x0; regs->x1 = response.x1;
    regs->x2 = response.x2; regs->x3 = response.x3;
    regs->x4 = response.x4; regs->x5 = response.x5;
    regs->x6 = response.x6; regs->x7 = response.x7;

    bool success = fault_advance_vcpu(vcpu_id, regs);
    assert(success);

    return success;
}
#endif

static smc_sip_handler_t smc_sip_handler = NULL;

bool smc_register_sip_handler(smc_sip_handler_t handler) {
    if (smc_sip_handler) {
        LOG_VMM_ERR("SMC SiP handler already registered\n");
        return false;
    }
    smc_sip_handler = handler;

    return true;
}

// @ivanv: print out which SMC call as a string we can't handle.
bool smc_handle(size_t vcpu_id, uint32_t hsr)
{
    // @ivanv: An optimisation to be made is to store the TCB registers so we don't
    // end up reading them multiple times
    seL4_UserContext regs;
    int err = seL4_TCB_ReadRegisters(BASE_VM_TCB_CAP + vcpu_id, false, 0, SEL4_USER_CONTEXT_SIZE, &regs);
    assert(err == seL4_NoError);

    size_t fn_number = smc_get_function_number(&regs);
    smc_call_id_t service = smc_get_call(regs.x0);

    switch (service) {
        case SMC_CALL_STD_SERVICE:
            if (fn_number < PSCI_MAX) {
                return handle_psci(vcpu_id, &regs, fn_number, hsr);
            }
            LOG_VMM_ERR("Unhandled SMC: standard service call %lu\n", fn_number);
            break;
        case SMC_CALL_SIP_SERVICE:
            if (smc_sip_handler) {
                return smc_sip_handler(vcpu_id, &regs, fn_number);
            }
            /* If we don't have a SiP handler registered, drop to the default case. */
        default:
            LOG_VMM_ERR("Unhandled SMC: unknown value service: 0x%lx, function number: 0x%lx\n", service, fn_number);
            break;
    }

    return false;
}
