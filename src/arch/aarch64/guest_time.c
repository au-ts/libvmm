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
#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/aarch64/guest_time.h>

#include "interfaces/sel4_client.h"
#include "sel4/sel4_arch/constants.h"

#include <sddf/timer/config.h>

extern timer_client_config_t timer_config;

/* ARCH timer reference */


/* Enough for EL1 virtual timer... */
#define MAX_CONCURRENT_TIMEOUT 1

typedef struct virtual_timer_time_out {
    bool valid;
    uint64_t absolute_expiry;
    size_t cookie;
    guest_timeout_callback_t callback_fn;
} virtual_timer_time_out_t;

struct guest_timekeeping {
    bool valid;

    microkit_channel timer_ch;
    virtual_timer_time_out_t timeouts[MAX_CONCURRENT_TIMEOUT];

    /* What is the timer state when it was last primed? */
    bool timer_primed;
    uint64_t time_at_timer_prime;
    uint64_t time_ticks_to_timeout;
};

static struct guest_timekeeping guest_timekeeping;

extern guest_t guest;

/* ------------- ARM EL1 virtual timer -------------*/
static inline uint64_t guest_cntfrq_hz(void)
{
    uint64_t freq;
    asm volatile("isb; mrs %0, cntfrq_el0" : "=r"(freq));
    return freq;
}

// `CNTV_CVAL_EL0` - (`CNTPCT_EL0`

static inline uint64_t guest_cntpct_el0(void)
{
    uint64_t cntpct_el0;
    asm volatile("isb; mrs %0, cntpct_el0" : "=r"(cntpct_el0));
    return cntpct_el0;
}

static inline uint64_t guest_current_time_ns(void)
{
    uint64_t current_time = guest_cntpct_el0() * 1000000000ULL / guest_cntfrq_hz();
    // LOG_VMM("Current time %lu\n", current_time);
    return current_time;
}

uint64_t guest_vtimer_remaining_time(size_t vcpu_id)
{
    uint64_t cval = microkit_vcpu_arm_read_reg(vcpu_id, seL4_VCPUReg_CNTV_CVAL);
    uint64_t now  = guest_cntpct_el0();
    if (cval <= now) {
        return 0;                 
    }
    uint64_t ticks = cval - now;
    return ticks * 1000000000ULL / guest_cntfrq_hz();
}

bool guest_time_init(microkit_channel timer_ch)
{
    guest_timekeeping.timer_ch = timer_ch;
    for (int i = 0; i < MAX_CONCURRENT_TIMEOUT; i++) {
        guest_timekeeping.timeouts[i].valid = false;
    }

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

uint64_t guest_time_now(void)
{
    guest_time_user_error_check();
    // This is a whole PPC (~1000 cycles for an IPC?) Pretty costly
    // We could just read the register instead of relying on the SDDF
    // LOG_VMM("sddf timer timer %lu\n", sddf_timer_time_now(guest_timekeeping.timer_ch));
    // return sddf_timer_time_now(guest_timekeeping.timer_ch);
    return guest_current_time_ns();
}

// static int guest_time_get_soonest_idx(void)
// {
//     guest_time_user_error_check();

//     uint64_t min_absolute_expiry = UINT64_MAX;
//     int min_idx = -1;
//     for (int i = 0; i < MAX_CONCURRENT_TIMEOUT; i++) {
//         if (guest_timekeeping.timeouts[i].valid) {
//             if (guest_timekeeping.timeouts[i].absolute_expiry < min_absolute_expiry) {
//                 min_absolute_expiry = guest_timekeeping.timeouts[i].absolute_expiry;
//                 min_idx = i;
//             }
//         }
//     }
//     return min_idx;
// }

static void guest_time_service_timeouts(void)
{
    uint64_t time_now = guest_time_now();
    for (int i = 0; i < MAX_CONCURRENT_TIMEOUT; i++) {
        if (guest_timekeeping.timeouts[i].valid) {
            // LOG_VMM("Checking %lu <= %lu\n", guest_timekeeping.timeouts[i].absolute_expiry, time_now);
            if (guest_timekeeping.timeouts[i].absolute_expiry <= time_now) {
                guest_timekeeping.timeouts[i].valid = false;
                guest_timekeeping.timeouts[i].callback_fn(guest_timekeeping.timeouts[i].cookie);
            }
        }
    }
}

static void guest_time_set_timeout(uint64_t time_delta, uint64_t time_now)
{
    guest_timekeeping.time_ticks_to_timeout = time_delta;
    guest_timekeeping.timer_primed = true;
    guest_timekeeping.time_at_timer_prime = time_now;

    sddf_timer_set_timeout(timer_config.driver_id,time_delta);
}

/* Program the sDDF driver with the next pending timeout if that is before what it is currently tracking, or it is tracking nothing*/
/* Assuming a single vCPU */
// static void guest_time_schedule_next(void)
// {
//     return;
// }

// static void guest_time_schedule_next(void)
// {
//     guest_time_user_error_check();

//     /* What is the soonest timeout to be concerned with? */
//     int soonest_timeout_idx = guest_time_get_soonest_idx();
//     if (soonest_timeout_idx == -1) {
//         return;
//     }

//     uint64_t soonest_absolute_expiry = guest_timekeeping.timeouts[soonest_timeout_idx].absolute_expiry;
//     uint64_t time_now = guest_time_now();
//     uint64_t time_ticks_to_timeout = soonest_absolute_expiry - time_now;

//     /* Force immediate VM exit upon entry. Is there a faster way to do this? What happens if we just return to the guest. */
//     if (time_now >= soonest_absolute_expiry) {
//         guest_time_set_timeout(0);
//         return;
//     }

//     bool timer_need_update = false;
//     if (!guest_timekeeping.timer_primed) {
//         timer_need_update = true;
//     } else {
//         /* There is a pending timeout. How many time ticks have passed since the timer was last primed,
//          * and how long until said timeout? */
//         uint64_t elapsed_time = time_now - guest_timekeeping.time_at_timer_prime;
//         uint64_t time_ticks_remaining_until_next_timeout = guest_timekeeping.time_ticks_to_timeout - elapsed_time;

//         if (elapsed_time >= guest_timekeeping.time_ticks_to_timeout) {
//             /* Timer will expire on next VM entry, no need to do anything. */
//             return;
//         }

//         if (time_ticks_to_timeout < time_ticks_remaining_until_next_timeout) {
//             /* Need an earlier timeout. */
//             timer_need_update = true;
//         }
//     }

//     if (timer_need_update) {
//         guest_time_set_timeout(time_ticks_to_timeout);
//     }
// }

guest_timeout_handle_t guest_time_request_timeout(uint64_t time_delta, guest_timeout_callback_t callback_fn,
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

    uint64_t time_now = guest_time_now();
    guest_timekeeping.timeouts[free_slot].absolute_expiry = time_now + time_delta;
    guest_timekeeping.timeouts[free_slot].callback_fn = callback_fn;
    guest_timekeeping.timeouts[free_slot].cookie = cookie;
    guest_timekeeping.timeouts[free_slot].valid = true;

    // LOG_VMM("Created timeout with absoulte expirery %lu, time delta %lu\n", 
        // guest_timekeeping.timeouts[free_slot].absolute_expiry, time_delta);

    /* Prime the timer if needed. */
    // guest_time_schedule_next();
    guest_time_set_timeout(time_delta, time_now);
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

    return true;
}

void guest_time_handle_timer_ntfn(void)
{
    guest_time_user_error_check();
    guest_timekeeping.timer_primed = false;

    guest_time_service_timeouts();
}