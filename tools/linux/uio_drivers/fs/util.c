/*
 * Copyright 2024, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <assert.h>

#include <lions/fs/protocol.h>

extern struct fs_queue *comp_queue;

void fs_queue_reply(fs_cmpl_t cmpl) {
    assert(fs_queue_length_producer(comp_queue) != FS_QUEUE_CAPACITY);
    fs_queue_idx_empty(comp_queue, 0)->cmpl = cmpl;
    fs_queue_publish_production(comp_queue, 1);
}
