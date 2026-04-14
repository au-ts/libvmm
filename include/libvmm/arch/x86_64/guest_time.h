/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>

/* Manage guest's perspective of time and the various timer devices' timeouts.
 * The guest will experience "wall clock" time. That is, guest-visible time sources such as the
 * TSC or HPET's main counter will continue to advance when the VCPU is preempted. */

#define TIMEOUT_HANDLE_INVALID -1

typedef void (*guest_timeout_callback_t)(size_t cookie);
typedef int guest_timeout_handle_t;

/* Initialises the guest time library. */
bool initialise_guest_time(microkit_channel timer_ch);

/* Returns the host CPU's TSC frequency as measured or detected. */
uint64_t guest_time_tsc_hz(void);

/* Returns what TSC value the guest should see right now, i.e. "the guest's time". */
uint64_t guest_time_tsc_now(void);

/* Requests for `callback_fn` to be called once TSC have ticked for `tsc_delta` ticks. */
guest_timeout_handle_t guest_time_request_timeout(uint64_t tsc_delta, guest_timeout_callback_t callback_fn,
                                                  size_t cookie);

bool guest_time_cancel_timeout(guest_timeout_handle_t handle);

/* Call this in your VMM's `notified()`! */
void guest_time_handle_timer_ntfn(void);
