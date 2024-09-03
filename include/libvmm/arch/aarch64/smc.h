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
bool smc_handle(size_t vcpu_id, uint32_t hsr);

/*
 * A custom handler for SMC SiP calls can be registered.
 * By default SiP calls are not handled.
 * Note only one handler can be registered at a given time.
 * smc_register_sip_handler returns false if a handler already exists.
 */
typedef bool (*smc_sip_handler_t)(size_t vcpu_id, seL4_UserContext *regs, size_t fn_number);
bool smc_register_sip_handler(smc_sip_handler_t handler);

#if defined(CONFIG_ALLOW_SMC_CALLS)
/*
 * Handle SMC SiP calls simply by forwarding the arguments to seL4 to perform
 * and placing the response registers into the guest's TCB registers.
 */
bool smc_sip_forward(size_t vcpu, seL4_UserContext *regs, size_t fn_number);
#endif

/* Helper functions */
void smc_set_return_value(seL4_UserContext *u, uint64_t val);

/* Gets the value of x1-x6 */
uint64_t smc_get_arg(seL4_UserContext *u, uint64_t arg);
