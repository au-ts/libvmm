/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <libvmm/virq.h>
#include <libvmm/virtio/virtq.h>
#include <libvmm/virtio/virtio.h>

/* An "iterator" over a descriptor chain that can contain both
 * ordinary buffers and indirect buffers tables. */

typedef struct virtio_desc_chain_iterator {
    virtio_queue_handler_t *vq_handler;
    uint16_t curr_desc;
    size_t steps;
    bool in_indrect;
    size_t curr_indirect_pos;
    size_t indirect_steps;
    size_t indirect_table_entries;
    bool terminated;
} virtio_desc_chain_iterator_t;

/* Given a descriptor head and a handle to the virtqueue, create a
 * forward iterator of the chain. */
bool virtio_desc_chain_iterator_new(virtio_queue_handler_t *vq_handler, uint16_t desc_head,
                                    virtio_desc_chain_iterator_t *ret);

typedef enum virtio_desc_chain_iterator_status {
    VIRTIO_ITERATOR_ERROR = 1, /* Chain is malformed */
    VIRTIO_ITERATOR_MOVED,     /* The iterator had yielded a result */
    VIRTIO_ITERATOR_EXHAUSTED, /* Nothing more. */
} virtio_desc_chain_iterator_status_t;

virtio_desc_chain_iterator_status_t virtio_desc_chain_iterator_next(virtio_desc_chain_iterator_t *iter, uint64_t *gpa,
                                                                    size_t *len);
