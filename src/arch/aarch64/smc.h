/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <microkit.h>

/* SMC vCPU fault handler */
bool handle_smc(size_t vcpu_id, uint32_t hsr);

/* Helper functions */
void smc_set_return_value(seL4_UserContext *u, uint64_t val);

/* Gets the value of x1-x6 */
uint64_t smc_get_arg(seL4_UserContext *u, uint64_t arg);

#if defined(CONFIG_ALLOW_SMC_CALLS)

/* Default handler of calls to a SMC Service */
bool smc_default_handle_service(size_t vcpu_id, seL4_UserContext *regs, uint64_t fn_number);

/* Service handler type */
typedef bool (* smc_handle_service_type)(size_t, seL4_UserContext *, uint64_t);

/* API for user: */

/* Set SMC Call Cap to use by the library API */
bool smc_set_cap(seL4_CPtr smcccap);

/* 'set_handler_sip_service()' assigns/removes active SiP Service handler */
bool smc_set_handler_sip_service(smc_handle_service_type handler_func);

#endif
