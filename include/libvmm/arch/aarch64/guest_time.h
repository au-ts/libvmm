/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <microkit.h>

/* Manage a guest's perspective of time and the various timer devices' timeouts. 
 * Currently the split with wfi..h is not super clear but hopefully could be refactored */

#define TIMEOUT_HANDLE_INVALID -1

/* Used to reference a timeout. >= 0 is valid, -1 is invalid*/
typedef int guest_timeout_handle_t;

typedef void (*guest_timeout_callback_t)(size_t cookie);

/* Initialises the guest time library. Must be called once before any other guest time calls */
bool guest_time_init(microkit_channel timer_ch);

/* Returns what value the guest should see right now, i.e. "the guest's time". */
uint64_t guest_time_now(void);

/* Requests for `callback_fn` to be called once the timer has ticked for `time_delta` ticks. */
guest_timeout_handle_t guest_time_request_timeout(uint64_t time_delta, guest_timeout_callback_t callback_fn, size_t cookie);

/* Cancel a pending timeout. Does error checking so safe if it has already fired */
bool guest_time_cancel_timeout(guest_timeout_handle_t handle);

/* Handles a notification from the timer, service timeouts and calling their callback functions */
void guest_time_handle_timer_ntfn(void);

/* The time remaining in the guests smallest virtual time, e.g when the next interrupt will fire */
uint64_t guest_vtimer_remaining_time(size_t vcpu_id);