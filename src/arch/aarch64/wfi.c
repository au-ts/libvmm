/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

// #include "interfaces/sel4_client.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <sel4/sel4.h>
#include <sddf/util/util.h>
#include <libvmm/util/util.h>
#include <libvmm/guest.h>
#include <libvmm/arch/aarch64/fault.h>
#include <libvmm/arch/aarch64/vgic/vgic.h>
#include <libvmm/arch/aarch64/guest_time.h>
#include <libvmm/arch/aarch64/wfi.h>

#define WFI_MAX_VCPUS 1

#ifndef NOWFI

typedef struct {
    bool suspended;
    guest_timeout_handle_t wake_timeout;
} wfi_vcpu_state_t;

static wfi_vcpu_state_t wfi_state[WFI_MAX_VCPUS];

void guest_wfi_init(void)
{
    for (int i = 0; i < WFI_MAX_VCPUS; i++) {
        wfi_state[i].suspended = false;
        wfi_state[i].wake_timeout = -1;
    }
}

/* The timer fired, so we need to inject the vIRQ into the guest and resume it 
 * Or if we just resume it the call will fire anyway? 
 * Pass in th vcpu as the cookie
*/
void wfi_timer_wake_cb(size_t cookie)
{
    size_t vcpu_id = cookie;

    if (!wfi_state[vcpu_id].suspended) {
        return;
    }

    wfi_state[vcpu_id].wake_timeout = -1;

    // LOG_VMM("Callback function activated on vcpu_id: %lu\n", vcpu_id);

    // if (!vgic_inject_irq(vcpu_id, 27)) {
    //     LOG_VMM_ERR("failed to inject vtimer PPI on WFI wake, vCPU %lu\n", vcpu_id);
    // }

    wfi_state[vcpu_id].suspended = false;

    seL4_TCB_Resume(BASE_VM_TCB_CAP + vcpu_id);

    return;
}

/* Called from fault.c */
bool guest_wfi_handle(size_t vcpu_id, uint64_t hsr, seL4_UserContext *regs)
{
    // LOG_VMM("WFI handle\n");
    if (vcpu_id >= WFI_MAX_VCPUS) {
        LOG_VMM_ERR("WFx on out-of-range vCPU %lu\n", vcpu_id);
        return false;
    }

    // We know we have a WFI/WFE, how to tell which is which

    /* Determine the wake time 
     * It could be based on the guests vtimer 
     * Or on waiting for an external interrupt. Do we need a saftey timer call?
     */
    uint64_t time_until_wake = guest_vtimer_remaining_time(vcpu_id);
    // LOG_VMM("%lu until wake for %lu\n", time_until_wake, vcpu_id);

    if (time_until_wake >= 10000000000ULL) {
        time_until_wake = 10000000000ULL;
    }

    /*
     * Advance the PC past the WFI. Not sure if we should do it now or when we resume.
     */
    if (!fault_advance_vcpu(vcpu_id, regs)) {
        return false;
    }

    if (time_until_wake == 0) {
        return true;
    }

    /* We ideally should never have the case where this happens, would have more vcpu than timeout slots*/
    guest_timeout_handle_t h = guest_time_request_timeout(time_until_wake, wfi_timer_wake_cb, vcpu_id);
    if (h < 0) {
        LOG_VMM_ERR("no timeout slot for WFI wake, vCPU %lu; not suspending\n", vcpu_id);
        return true;
    }

    wfi_state[vcpu_id].wake_timeout = h;
    wfi_state[vcpu_id].suspended = true;

    seL4_TCB_Suspend(BASE_VM_TCB_CAP + vcpu_id);
    return true;
}

void guest_wfi_resume_if_suspended(size_t vcpu_id)
{
    /* Remove vcpu from any bookkeeping and resume the TCB */

    // Fault checking? Well we check even on a notification, so the guest may not be suspended

    if (!wfi_state[vcpu_id].suspended) {
        return;
    }

    wfi_state[vcpu_id].suspended = false;

    if (wfi_state[vcpu_id].wake_timeout >= 0) {
        guest_time_cancel_timeout(wfi_state[vcpu_id].wake_timeout);
        wfi_state[vcpu_id].wake_timeout = -1;
    }

    seL4_TCB_Resume(BASE_VM_TCB_CAP + vcpu_id);
}

#endif 