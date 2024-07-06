/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "smc.h"
#include "psci.h"
#include "fault.h"
#include "../../util/util.h"

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

#if defined(CONFIG_ALLOW_SMC_CALLS)
/* SMC Call Cap that the library makes SMC calls on; passed from an application */
seL4_CPtr smc_cap_current;

/* Set SMC Call Cap to use by the library API;
   should be called before an application starts
   handling exceptions from a VM thread
*/
bool smc_set_cap(seL4_CPtr smcccap)
{
    if (!smcccap)
    {
        LOG_VMM_ERR("SMC forwarding: attempted to set zero SMC Call cap\n");
        return false;
    }
    smc_cap_current = smcccap;
    return true;
}
#endif /*CONFIG_ALLOW_SMC_CALLS*/

#if defined(CONFIG_ALLOW_SMC_CALLS)

/* Service handlers for services that require "physical" access to EL3 components
   (vs emulation) are listed here.

   'Active' handler is a pointer to a fucntion that actually will be invoked;
   by default active handlers are set to NULL. Application should assign to an active handler
   wheither a default handler that unconditionally forwards calls to Secure Monitor:

        smc_set_handler_xxx_service(smc_default_handle_service);

   or assign a custom handler impelemnting some forwarding policy:

        smc_set_handler_xxx_service(wary_handle_service);

   To remove active handler invoke:

        smc_set_handler_xxx_service(NULL);

   Note: application code should include smc.h
*/

/* 'smc_handle_sip()' is an active handler of SiP Service calls */
smc_handle_service_type smc_handle_sip = NULL;

/* 'smc_set_handler_sip_service()' assigns/removes active SiP Service handler;
   NULL is a valid value to remove an earlier assigned handler;
   function returns 'True' if handler was initialized with pointer to a function,
   and 'False' if a handler was removed */
bool smc_set_handler_sip_service (smc_handle_service_type handler_func)
{
    if (handler_func == NULL)
    {
        smc_handle_sip = NULL;
        return false;
    }
    else
        smc_handle_sip = handler_func;

    return true;
}
#endif /*CONFIG_ALLOW_SMC_CALLS*/

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

// @ivanv: print out which SMC call as a string we can't handle.
bool handle_smc(size_t vcpu_id, uint32_t hsr)
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
#if defined(CONFIG_ALLOW_SMC_CALLS)
        case SMC_CALL_SIP_SERVICE:
            return smc_handle_sip(vcpu_id, &regs, fn_number);
#endif
        default:
            LOG_VMM_ERR("Unhandled SMC: unknown value service: 0x%lx, function number: 0x%lx\n", service, fn_number);
            break;
    }

    return false;
}

#if defined(CONFIG_ALLOW_SMC_CALLS)
/* Default handler of calls to a Service (group of Functions) running at Secure Monitor level;
  forwards all the calls without applying any policy
*/
bool smc_default_handle_service(size_t vcpu_id, seL4_UserContext *regs, uint64_t fn_number)
{

    seL4_ARM_SMCContext request;
    seL4_ARM_SMCContext response;

    request.x0 = regs->x0; request.x1 = regs->x1;
    request.x2 = regs->x2; request.x3 = regs->x3;
    request.x4 = regs->x4; request.x5 = regs->x5;
    request.x6 = regs->x6; request.x7 = regs->x7;

    seL4_ARM_SMC_Call(smc_cap_current, &request, &response);

    regs->x0 = response.x0; regs->x1 = response.x1;
    regs->x2 = response.x2; regs->x3 = response.x3;
    regs->x4 = response.x4; regs->x5 = response.x5;
    regs->x6 = response.x6; regs->x7 = response.x7;


    bool success = fault_advance_vcpu(vcpu_id, regs);
    assert(success);

    return success;
}
#endif /*CONFIG_ALLOW_SMC_CALLS*/
