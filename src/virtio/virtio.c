/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <libvmm/virtio/virtq.h>
#include <libvmm/virtio/virtio.h>

uint64_t virtio_desc_chain_payload_len(virtio_queue_handler_t *vq_handler, uint16_t desc_head)
{
    assert(vq_handler->ready);
    struct virtq *virtq = &vq_handler->virtq;

    uint64_t loop_iter_count = 0;
    uint64_t payload_len = 0;
    uint16_t curr_desc = desc_head;
    while (true) {
        if (loop_iter_count > virtq->num || curr_desc >= virtq->num) {
            LOG_VMM_ERR("bad descriptor chain starting at %u\n", desc_head);
            return 0;
        }

        payload_len += virtq->desc[curr_desc].len;
        if (!(virtq->desc[curr_desc].flags & VIRTQ_DESC_F_NEXT)) {
            break;
        }

        curr_desc = virtq->desc[curr_desc].next;
        loop_iter_count++;
    };
    return payload_len;
}

bool virtio_read_data_from_desc_chain(virtio_queue_handler_t *vq_handler, uint16_t desc_head, uint64_t bytes_to_read,
                                      uint64_t read_off, char *data)
{
    assert(vq_handler->ready);
    struct virtq *virtq = &vq_handler->virtq;

    uint64_t current_list_byte = read_off;
    uint64_t end_list_byte = read_off + bytes_to_read;
    uint64_t current_desc_start_byte = 0;

    uint16_t curr_desc = desc_head;
    uint64_t loop_iter_count = 0;
    while (true) {
        if (loop_iter_count > virtq->num || curr_desc >= virtq->num) {
            LOG_VMM_ERR("bad descriptor chain starting at %u\n", desc_head);
            return false;
        }

        struct virtq_desc *desc = &virtq->desc[curr_desc];
        uint64_t current_desc_end_byte = current_desc_start_byte + desc->len;

        if (current_list_byte >= current_desc_start_byte && current_list_byte < current_desc_end_byte) {
            /* This descriptor have what we need, copy it over to `data`. */
            uint64_t copy_size = MIN(current_desc_end_byte, end_list_byte) - current_list_byte;
            uint64_t src_gpa = desc->addr + (current_list_byte - current_desc_start_byte);
            char *dest = data + (current_list_byte - read_off);

            memcpy(dest, (char *)src_gpa, copy_size);
            current_list_byte += copy_size;
        }

        assert(current_list_byte <= end_list_byte);
        if (current_list_byte == end_list_byte) {
            break;
        }
        loop_iter_count++;
        current_desc_start_byte += desc->len;
        curr_desc = virtq->desc[curr_desc].next;
    }

    return current_list_byte == end_list_byte;
}

bool virtio_write_data_to_desc_chain(virtio_queue_handler_t *vq_handler, uint16_t desc_head, uint64_t bytes_to_write,
                                     uint64_t write_off, char *data)
{
    assert(vq_handler->ready);
    struct virtq *virtq = &vq_handler->virtq;

    uint64_t current_list_byte = write_off;
    uint64_t end_list_byte = write_off + bytes_to_write;
    uint64_t current_desc_start_byte = 0;

    uint16_t curr_desc = desc_head;
    uint64_t loop_iter_count = 0;
    while (true) {
        if (loop_iter_count > virtq->num || curr_desc >= virtq->num) {
            LOG_VMM_ERR("bad descriptor chain starting at %u\n", desc_head);
            return false;
        }

        struct virtq_desc *desc = &virtq->desc[curr_desc];
        uint64_t current_desc_end_byte = current_desc_start_byte + desc->len;

        if (current_list_byte >= current_desc_start_byte && current_list_byte < current_desc_end_byte) {
            /* This descriptor have what we need, copy `data` to guest RAM. */
            uint64_t copy_size = MIN(current_desc_end_byte, end_list_byte) - current_list_byte;
            char *src = data + (current_list_byte - write_off);
            uint64_t dest_gpa = desc->addr + (current_list_byte - current_desc_start_byte);

            memcpy((char *)dest_gpa, src, copy_size);
            current_list_byte += copy_size;
        }

        assert(current_list_byte <= end_list_byte);
        if (current_list_byte == end_list_byte) {
            break;
        }

        loop_iter_count++;
        current_desc_start_byte += desc->len;
        curr_desc = virtq->desc[curr_desc].next;
    }

    return true;
}

bool virtio_virtq_peek_avail(virtio_queue_handler_t *vq_handler, uint16_t *ret)
{
    assert(vq_handler->ready);
    struct virtq *virtq = &vq_handler->virtq;

    if (vq_handler->last_idx != virtq->avail->idx) {
        *ret = virtq->avail->ring[vq_handler->last_idx % virtq->num];
        return true;
    }

    return false;
}

bool virtio_virtq_pop_avail(virtio_queue_handler_t *vq_handler, uint16_t *ret)
{
    assert(vq_handler->ready);

    uint16_t desc_head;
    bool available = virtio_virtq_peek_avail(vq_handler, &desc_head);

    if (available) {
        *ret = desc_head;
        vq_handler->last_idx++;
    }

    return available;
}

void virtio_virtq_add_used(virtio_queue_handler_t *vq_handler, uint16_t desc_head, uint32_t len)
{
    assert(vq_handler->ready);
    struct virtq *virtq = &vq_handler->virtq;

    struct virtq_used_elem *used_elem = &virtq->used->ring[virtq->used->idx % virtq->num];
    used_elem->id = desc_head;
    used_elem->len = len;
    virtq->used->idx++;
}
