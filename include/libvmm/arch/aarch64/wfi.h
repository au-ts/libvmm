/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* Worth adding detail here about the flow? */ 

#pragma once
 
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sel4/sel4.h>
 
void guest_wfi_init(void);
 
/*
 * Handle a guests call to WFI. 
 */
bool guest_wfi_handle(size_t vcpu_id, uint64_t hsr, seL4_UserContext *regs);
 
/*
 * Resume a vCPU if it is currently WFI-suspended, cancelling its pending wake
 * timeout.
 */
void guest_wfi_resume_if_suspended(size_t vcpu_id);