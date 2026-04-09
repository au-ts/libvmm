/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Given a descriptor head, walk the descriptor chain and compute the sum of the length of all the desciptor chain's buffers */
uint64_t virtio_desc_chain_payload_len(virtio_queue_handler_t *vq_handler, uint16_t desc_head);

/* Given a scatter gather list of a virtio request in the form of a descriptor head, copy a chunk of data from the list,
 * starting from `read_off` into `data`. For example, if you want the body of a virtio block read or write request,
 * `read_off` should be `sizeof(struct virtio_blk_outhdr)`.
 * Return true if the operation completed successfully, false if the amount of data overflows the
 * scatter gather list. */
bool virtio_read_data_from_desc_chain(virtio_queue_handler_t *vq_handler, uint16_t desc_head, uint64_t bytes_to_read,
                                      uint64_t read_off, char *data);

/* Same idea as above but we are now writing to the scatter gather buffers in guest RAM */
bool virtio_write_data_to_desc_chain(virtio_queue_handler_t *vq_handler, uint16_t desc_head, uint64_t bytes_to_write,
                                     uint64_t write_off, char *data);

/* Peek an available descriptor head from a virtq */
bool virtio_virtq_peek_avail(virtio_queue_handler_t *vq_handler, uint16_t *ret);

/* Pop an available descriptor head from a virtq */
bool virtio_virtq_pop_avail(virtio_queue_handler_t *vq_handler, uint16_t *ret);

/* Given a descriptor head and the number of bytes that were written to it by the VMM, place it
 * in the used queue */
void virtio_virtq_add_used(virtio_queue_handler_t *vq_handler, uint16_t desc_head, uint32_t bytes_written);