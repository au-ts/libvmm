/*
 * Copyright 2022, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "include/shared_ringbuffer.h"

void ring_init(ring_handle_t *ring, ring_buffer_t *avail, ring_buffer_t *used, notify_fn notify, int buffer_init)
{
    ring->used_ring = used;
    ring->avail_ring = avail;
    ring->notify = notify;

    if (buffer_init) {
        ring->used_ring->write_idx = 0;
        ring->used_ring->read_idx = 0;
        ring->avail_ring->write_idx = 0;
        ring->avail_ring->read_idx = 0;
    }
}
