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

/* By default the clocksource and calculation unit is always TSC. But the timeout mechanism can be
 * configured to use either the sDDF timer timeout or the VMX-preemption timer.
 * Right now VMX-preemption timer does not work for some reasons and isn't
 * officially supported by seL4 anyways. As the kernel does not allow us to read IA32_VMX_MISC
 * which contains data needed for us to compute the VMX-preemption timer frequency. */

// #define GUEST_TIME_USE_VMX_TIMER

#define IA32_VMX_MISC 0x485

/* Enough for local APIC timer + 3 HPET comparators, increase this if you add more timer devices. */
#define MAX_CONCURRENT_TIMEOUT 4

struct virtual_timer_time_out {
    bool valid;
    uint64_t absolute_expiry_tsc;
    size_t cookie;
    guest_timeout_callback_t callback_fn;
} virtual_timer_time_out_t;

struct guest_timekeeping {
    bool valid;

    uint64_t tsc_hz;
    microkit_channel timer_ch;
    struct virtual_timer_time_out timeouts[MAX_CONCURRENT_TIMEOUT];

    /* VMX-preemption timer only ticks after 2^vmx_timer_shift TSC ticks */
    uint8_t vmx_timer_shift;

    /* What is the timer state when it was last primed? */
    bool timer_primed;
    uint64_t tsc_at_timer_prime;
    uint64_t tsc_ticks_to_timeout;
};

static struct guest_timekeeping guest_timekeeping;

bool initialise_guest_time(microkit_channel timer_ch)
{
    guest_timekeeping.timer_ch = timer_ch;
    guest_timekeeping.tsc_hz = get_host_tsc_hz(timer_ch);
    for (int i = 0; i < MAX_CONCURRENT_TIMEOUT; i++) {
        guest_timekeeping.timeouts[i].valid = false;
    }

#if defined(GUEST_TIME_USE_VMX_TIMER)
    guest_timekeeping.vmx_timer_shift = seL4_X86DangerousRDMSR(IA32_VMX_MISC) & 0x1f;
    LOG_VMM("vmx timer shift is %u\n", guest_timekeeping.vmx_timer_shift);
#endif

    guest_timekeeping.valid = true;
    return true;
}

static inline void guest_time_user_error_check(void)
{
    if (!guest_timekeeping.valid) {
        LOG_VMM_ERR("%s called before initialise_guest_time()!\n", __func__);
        assert(false);
    }
}

uint64_t guest_time_tsc_hz(void)
{
    guest_time_user_error_check();
    return guest_timekeeping.tsc_hz;
}

uint64_t guest_time_tsc_now(void)
{
    guest_time_user_error_check();
    return rdtsc();
}

static int guest_time_get_soonest_absolute_expiry_tsc(void)
{
    guest_time_user_error_check();

    uint64_t min_absolute_expiry_tsc = UINT64_MAX;
    int min_idx = -1;
    for (int i = 0; i < MAX_CONCURRENT_TIMEOUT; i++) {
        if (guest_timekeeping.timeouts[i].valid) {
            if (guest_timekeeping.timeouts[i].absolute_expiry_tsc < min_absolute_expiry_tsc) {
                min_absolute_expiry_tsc = guest_timekeeping.timeouts[i].absolute_expiry_tsc;
                min_idx = i;
            }
        }
    }
    return min_idx;
}

static void guest_time_set_timeout(uint64_t tsc_delta)
{
    guest_timekeeping.tsc_ticks_to_timeout = tsc_delta;
    guest_timekeeping.timer_primed = true;
    guest_timekeeping.tsc_at_timer_prime = guest_time_tsc_now();

#if defined(GUEST_TIME_USE_VMX_TIMER)
    uint64_t vmx_timer_value = guest_timekeeping.tsc_ticks_to_timeout >> guest_timekeeping.vmx_timer_shift;
    if (vmx_timer_value > UINT32_MAX) {
        LOG_VMM_ERR("VMX-preemption timer value 0x%lx, from TSC ticks to time out 0x%lx, exceeds 32-bit!\n",
                    vmx_timer_value, guest_timekeeping.tsc_ticks_to_timeout);
        assert(false);
    }
    microkit_vcpu_x86_write_vmcs(0, VMX_GUEST_PREEMPTION_TIMER_VALUE, vmx_timer_value);
    /* We don't need read-set-write as the kernel does that for us. */
    microkit_vcpu_x86_write_vmcs(0, VMX_CONTROL_PIN_EXECUTION_CONTROLS, BIT(6));
#else
    uint64_t delay_ns = convert_ticks_by_frequency(guest_timekeeping.tsc_ticks_to_timeout, guest_time_tsc_hz(),
                                                   NS_IN_S);
    sddf_timer_set_timeout(guest_timekeeping.timer_ch, delay_ns);
#endif
}

static void guest_time_schedule_timeout(void)
{
    guest_time_user_error_check();

    /* What is the soonest timeout to be concerned with? */
    int soonest_timeout_idx = guest_time_get_soonest_absolute_expiry_tsc();
    if (soonest_timeout_idx == -1) {
        return;
    }
    uint64_t soonest_absolute_expiry_tsc = guest_timekeeping.timeouts[soonest_timeout_idx].absolute_expiry_tsc;

    uint64_t tsc_now = guest_time_tsc_now();
    if (soonest_absolute_expiry_tsc <= tsc_now) {
        return;
    }

    bool timer_need_update = false;
    uint64_t tsc_ticks_to_timeout;
    if (!guest_timekeeping.timer_primed) {
        timer_need_update = true;
        tsc_ticks_to_timeout = soonest_absolute_expiry_tsc - tsc_now;
    } else {
        /* There is a pending timeout. How many TSC ticks have passed since the timer was last primed,
         * and how long until said timeout? */
        uint64_t elapsed_tsc = tsc_now - guest_timekeeping.tsc_at_timer_prime;
        uint64_t tsc_ticks_remaining_until_next_timeout = guest_timekeeping.tsc_ticks_to_timeout - elapsed_tsc;

        if ((soonest_absolute_expiry_tsc - tsc_now) < tsc_ticks_remaining_until_next_timeout) {
            /* Need an earlier timeout. */
            timer_need_update = true;
            tsc_ticks_to_timeout = soonest_absolute_expiry_tsc - tsc_now;
        }
    }

    if (timer_need_update) {
        guest_time_set_timeout(tsc_ticks_to_timeout);
    }
}

guest_timeout_handle_t guest_time_request_timeout(uint64_t tsc_delta, guest_timeout_callback_t callback_fn,
                                                  size_t cookie)
{
    guest_time_user_error_check();

    /* Find a free bookkeeping slot. */
    int free_slot = 0;
    for (; free_slot < MAX_CONCURRENT_TIMEOUT; free_slot++) {
        if (!guest_timekeeping.timeouts[free_slot].valid) {
            break;
        }
    }

    if (free_slot == MAX_CONCURRENT_TIMEOUT) {
        return -1;
    }

    guest_timekeeping.timeouts[free_slot].absolute_expiry_tsc = guest_time_tsc_now() + tsc_delta;
    guest_timekeeping.timeouts[free_slot].callback_fn = callback_fn;
    guest_timekeeping.timeouts[free_slot].cookie = cookie;
    guest_timekeeping.timeouts[free_slot].valid = true;

    /* Prime the timer if needed. */
    guest_time_schedule_timeout();
    return free_slot;
}

bool guest_time_cancel_timeout(guest_timeout_handle_t handle)
{
    guest_time_user_error_check();

    if (handle >= MAX_CONCURRENT_TIMEOUT) {
        return false;
    }
    if (!guest_timekeeping.timeouts[handle].valid) {
        return false;
    }
    guest_timekeeping.timeouts[handle].valid = false;

#if defined(GUEST_TIME_USE_VMX_TIMER)
    microkit_vcpu_x86_write_vmcs(0, VMX_CONTROL_PIN_EXECUTION_CONTROLS, 0);
#else
    /* Can't cancel a sDDF timeout so do nothing */
#endif

    return true;
}

void guest_time_handle_timer_ntfn(void)
{
    guest_time_user_error_check();
    guest_timekeeping.timer_primed = false;

#if defined(GUEST_TIME_USE_VMX_TIMER)
    microkit_vcpu_x86_write_vmcs(0, VMX_CONTROL_PIN_EXECUTION_CONTROLS, 0);
#endif

    uint64_t tsc_now = guest_time_tsc_now();
    for (int i = 0; i < MAX_CONCURRENT_TIMEOUT; i++) {
        if (guest_timekeeping.timeouts[i].valid) {
            if (guest_timekeeping.timeouts[i].absolute_expiry_tsc <= tsc_now) {
                guest_timekeeping.timeouts[i].valid = false;
                guest_timekeeping.timeouts[i].callback_fn(guest_timekeeping.timeouts[i].cookie);
            }
        }
    }

    guest_time_schedule_timeout();
}