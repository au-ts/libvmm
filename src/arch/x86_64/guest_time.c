/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <sddf/util/util.h>
#include <sddf/timer/client.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/tsc.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/guest_time.h>
#include <libvmm/arch/x86_64/vmm_state.h>

/* The clocksource and calculation unit is always TSC for the best performance. */
#define IA32_VMX_MISC 0x485

extern struct local_vmm_state local_vmm_state;

// @billn revisit removing timer_ch
// by implementing "setvar_tsc_freq" or similar in Microkit
bool initialise_guest_time(microkit_channel timer_ch)
{
    local_vmm_state.tsc_hz = get_host_tsc_hz(timer_ch);

    /* "The IA32_VMX_MISC MSR (index 485H) consists of the following fields:
        Bits 4:0 report a value X that specifies the relationship between the rate of the VMX-preemption timer and that
        of the timestamp counter (TSC). Specifically, the VMX-preemption timer (if it is active) counts down by 1 every
        time bit X in the TSC changes due to a TSC increment.
    */
    local_vmm_state.vmx_timer_shift = microkit_vcpu_x86_read_msr(0, IA32_VMX_MISC) & 0x1f;
    return true;
}

uint64_t guest_time_tsc_hz(void)
{
    return local_vmm_state.tsc_hz;
}

uint64_t guest_time_tsc_now(void)
{
    return rdtsc();
}

static int guest_time_get_soonest_absolute_expiry_tsc(void)
{
    uint64_t min_absolute_expiry_tsc = UINT64_MAX;
    int min_idx = -1;
    for (int i = 0; i < MAX_CONCURRENT_TIMEOUT; i++) {
        if (local_vmm_state.guest_timer_timeouts[i].valid) {
            if (local_vmm_state.guest_timer_timeouts[i].absolute_expiry_tsc < min_absolute_expiry_tsc) {
                min_absolute_expiry_tsc = local_vmm_state.guest_timer_timeouts[i].absolute_expiry_tsc;
                min_idx = i;
            }
        }
    }
    return min_idx;
}

static void guest_time_service_timeouts(void)
{
    uint64_t tsc_now = guest_time_tsc_now();
    for (int i = 0; i < MAX_CONCURRENT_TIMEOUT; i++) {
        if (local_vmm_state.guest_timer_timeouts[i].valid) {
            if (local_vmm_state.guest_timer_timeouts[i].absolute_expiry_tsc <= tsc_now) {
                local_vmm_state.guest_timer_timeouts[i].valid = false;
                local_vmm_state.guest_timer_timeouts[i].callback_fn(local_vmm_state.guest_timer_timeouts[i].cookie);
            }
        }
    }
}

static void guest_time_set_timeout(uint64_t tsc_delta)
{
    local_vmm_state.tsc_ticks_to_vmx_timeout = tsc_delta;
    local_vmm_state.vmx_timer_primed = true;
    local_vmm_state.tsc_at_vmx_timer_prime = guest_time_tsc_now();

    uint64_t vmx_timer_value = local_vmm_state.tsc_ticks_to_vmx_timeout >> local_vmm_state.vmx_timer_shift;
    if (vmx_timer_value > UINT32_MAX) {
        vmx_timer_value = UINT32_MAX;
        local_vmm_state.tsc_ticks_to_vmx_timeout = (uint64_t)UINT32_MAX << local_vmm_state.vmx_timer_shift;
    }

    microkit_vcpu_x86_write_vmcs(0, VMX_GUEST_PREEMPTION_TIMER_VALUE, vmx_timer_value);
    /* We don't need read-set-write as the kernel does that for us. */
    microkit_vcpu_x86_write_vmcs(0, VMX_CONTROL_PIN_EXECUTION_CONTROLS, BIT(6));
    microkit_vcpu_x86_write_vmcs(0, VMX_CONTROL_EXIT_CONTROLS, VMCS_VEXC_VMX_TIMER_ON);
}

static void guest_time_schedule_timeout(void)
{
    /* What is the soonest timeout to be concerned with? */
    int soonest_timeout_idx = guest_time_get_soonest_absolute_expiry_tsc();
    if (soonest_timeout_idx == -1) {
        return;
    }

    uint64_t soonest_absolute_expiry_tsc =
        local_vmm_state.guest_timer_timeouts[soonest_timeout_idx].absolute_expiry_tsc;
    uint64_t tsc_now = guest_time_tsc_now();
    uint64_t tsc_ticks_to_timeout = soonest_absolute_expiry_tsc - tsc_now;

    if (tsc_now >= soonest_absolute_expiry_tsc) {
        guest_time_set_timeout(0); // Force immediate VM exit upon entry
        return;
    }

    bool timer_need_update = false;
    if (!local_vmm_state.vmx_timer_primed) {
        timer_need_update = true;
    } else {
        /* There is a pending timeout. How many TSC ticks have passed since the timer was last primed,
         * and how long until said timeout? */
        uint64_t elapsed_tsc = tsc_now - local_vmm_state.tsc_at_vmx_timer_prime;
        uint64_t tsc_ticks_remaining_until_next_timeout = local_vmm_state.tsc_ticks_to_vmx_timeout - elapsed_tsc;

        if (elapsed_tsc >= local_vmm_state.tsc_ticks_to_vmx_timeout) {
            /* Timer will expire on next VM entry, no need to do anything. */
            return;
        }

        if (tsc_ticks_to_timeout < tsc_ticks_remaining_until_next_timeout) {
            /* Need an earlier timeout. */
            timer_need_update = true;
        }
    }

    if (timer_need_update) {
        guest_time_set_timeout(tsc_ticks_to_timeout);
    }
}

guest_timeout_handle_t guest_time_request_timeout(uint64_t tsc_delta, guest_timeout_callback_t callback_fn,
                                                  size_t cookie)
{
    /* Find a free bookkeeping slot. */
    int free_slot = 0;
    for (; free_slot < MAX_CONCURRENT_TIMEOUT; free_slot++) {
        if (!local_vmm_state.guest_timer_timeouts[free_slot].valid) {
            break;
        }
    }

    if (free_slot == MAX_CONCURRENT_TIMEOUT) {
        return -1;
    }

    local_vmm_state.guest_timer_timeouts[free_slot].absolute_expiry_tsc = guest_time_tsc_now() + tsc_delta;
    local_vmm_state.guest_timer_timeouts[free_slot].callback_fn = callback_fn;
    local_vmm_state.guest_timer_timeouts[free_slot].cookie = cookie;
    local_vmm_state.guest_timer_timeouts[free_slot].valid = true;

    /* Prime the timer if needed. */
    guest_time_schedule_timeout();
    return free_slot;
}

bool guest_time_cancel_timeout(guest_timeout_handle_t handle)
{
    if (handle >= MAX_CONCURRENT_TIMEOUT) {
        return false;
    }
    if (!local_vmm_state.guest_timer_timeouts[handle].valid) {
        return false;
    }
    local_vmm_state.guest_timer_timeouts[handle].valid = false;

    return true;
}

void guest_time_handle_timer_ntfn(void)
{
    local_vmm_state.vmx_timer_primed = false;

    /* Turn off the VMX-preemption timer. No need for read-clear-write as the kernel will do that for us,
     * this is only a problem if we set other bits somewhere else in the VMM but we don't right now. */
    microkit_vcpu_x86_write_vmcs(0, VMX_CONTROL_PIN_EXECUTION_CONTROLS, 0);
    microkit_vcpu_x86_write_vmcs(0, VMX_CONTROL_EXIT_CONTROLS, VMCS_VEXC_DEFAULT);

    guest_time_service_timeouts();

    guest_time_schedule_timeout();
}