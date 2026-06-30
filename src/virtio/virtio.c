/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <libvmm/guest_ram.h>
#include <libvmm/pci.h>
#include <libvmm/virtio/virtq.h>
#include <libvmm/virtio/virtio.h>

/* These are incorrect if VIRTIO_F_EVENT_IDX feature is negotitated, but we don't support that for now. */
static inline size_t virtio_desc_ring_size_bytes(struct virtq *virtq)
{
    return virtq->num * sizeof(struct virtq_desc);
}

static inline size_t virtio_avail_ring_size_bytes(struct virtq *virtq)
{
    /* There are 2 u16 before the actual ring. */
    return 4 + virtq->num * sizeof(uint16_t);
}

static inline size_t virtio_used_ring_size_bytes(struct virtq *virtq)
{
    /* There are 2 u16 before the actual ring. */
    return 4 + virtq->num * sizeof(struct virtq_used_elem);
}

struct virtq_desc *virtio_get_desc_ring(struct virtq *virtq)
{
    uint64_t desc_ring_gpa = (uint64_t)virtq->desc_gpa;
    return (struct virtq_desc *)gpa_to_hva(desc_ring_gpa, virtio_desc_ring_size_bytes(virtq));
}

struct virtq_avail *virtio_get_avail_ring(struct virtq *virtq)
{
    uint64_t avail_ring_gpa = (uint64_t)virtq->avail_gpa;
    return (struct virtq_avail *)gpa_to_hva(avail_ring_gpa, virtio_avail_ring_size_bytes(virtq));
}

struct virtq_used *virtio_get_used_ring(struct virtq *virtq)
{
    uint64_t used_ring_gpa = (uint64_t)virtq->used_gpa;
    return (struct virtq_used *)gpa_to_hva(used_ring_gpa, virtio_used_ring_size_bytes(virtq));
}

uint64_t virtio_desc_chain_payload_len(virtio_queue_handler_t *vq_handler, uint16_t desc_head)
{
    assert(vq_handler->ready);
    struct virtq *virtq = &vq_handler->virtq;
    struct virtq_desc *desc_ring = virtio_get_desc_ring(virtq);

    uint64_t loop_iter_count = 0;
    uint64_t payload_len = 0;
    uint16_t curr_desc = desc_head;
    while (true) {
        if (loop_iter_count > virtq->num || curr_desc >= virtq->num) {
            LOG_VMM_ERR("bad descriptor chain starting at %u\n", desc_head);
            return 0;
        }

        payload_len += desc_ring[curr_desc].len;
        if (!(desc_ring[curr_desc].flags & VIRTQ_DESC_F_NEXT)) {
            break;
        }

        curr_desc = desc_ring[curr_desc].next;
        loop_iter_count++;
    };
    return payload_len;
}

bool virtio_read_data_from_desc_chain(virtio_queue_handler_t *vq_handler, uint16_t desc_head, uint64_t bytes_to_read,
                                      uint64_t read_off, char *data)
{
    assert(vq_handler->ready);
    struct virtq *virtq = &vq_handler->virtq;
    struct virtq_desc *desc_ring = virtio_get_desc_ring(virtq);

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

        struct virtq_desc *desc = &desc_ring[curr_desc];
        uint64_t current_desc_end_byte = current_desc_start_byte + desc->len;

        if (current_list_byte >= current_desc_start_byte && current_list_byte < current_desc_end_byte) {
            /* This descriptor have what we need, copy it over to `data`. */
            uint64_t copy_size = MIN(current_desc_end_byte, end_list_byte) - current_list_byte;
            uint64_t src_gpa = desc->addr + (current_list_byte - current_desc_start_byte);
            void *src_hva = gpa_to_hva(src_gpa, copy_size);
            char *dest = data + (current_list_byte - read_off);

            memcpy(dest, src_hva, copy_size);
            current_list_byte += copy_size;
        }

        assert(current_list_byte <= end_list_byte);
        if (current_list_byte == end_list_byte) {
            break;
        }
        loop_iter_count++;
        current_desc_start_byte += desc->len;
        curr_desc = desc_ring[curr_desc].next;
    }

    return current_list_byte == end_list_byte;
}

bool virtio_write_data_to_desc_chain(virtio_queue_handler_t *vq_handler, uint16_t desc_head, uint64_t bytes_to_write,
                                     uint64_t write_off, char *data)
{
    assert(vq_handler->ready);
    struct virtq *virtq = &vq_handler->virtq;
    struct virtq_desc *desc_ring = virtio_get_desc_ring(virtq);

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

        struct virtq_desc *desc = &desc_ring[curr_desc];
        uint64_t current_desc_end_byte = current_desc_start_byte + desc->len;

        if (current_list_byte >= current_desc_start_byte && current_list_byte < current_desc_end_byte) {
            /* This descriptor have what we need, copy `data` to guest RAM. */
            uint64_t copy_size = MIN(current_desc_end_byte, end_list_byte) - current_list_byte;
            char *src = data + (current_list_byte - write_off);
            uint64_t dest_gpa = desc->addr + (current_list_byte - current_desc_start_byte);
            void *dest_hva = gpa_to_hva(dest_gpa, copy_size);

            memcpy(dest_hva, src, copy_size);
            current_list_byte += copy_size;
        }

        assert(current_list_byte <= end_list_byte);
        if (current_list_byte == end_list_byte) {
            break;
        }

        loop_iter_count++;
        current_desc_start_byte += desc->len;
        curr_desc = desc_ring[curr_desc].next;
    }

    return true;
}

bool virtio_virtq_peek_avail(virtio_queue_handler_t *vq_handler, uint16_t *ret)
{
    assert(vq_handler->ready);
    struct virtq *virtq = &vq_handler->virtq;
    struct virtq_avail *avail_ring = virtio_get_avail_ring(virtq);

    if (vq_handler->last_idx != avail_ring->idx) {
        *ret = avail_ring->ring[vq_handler->last_idx % virtq->num];
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
    struct virtq_used *used_ring = virtio_get_used_ring(virtq);

    struct virtq_used_elem *used_elem = &used_ring->ring[used_ring->idx % virtq->num];
    used_elem->id = desc_head;
    used_elem->len = len;
    used_ring->idx++;
}

void virtio_set_interrupt_status(struct virtio_device *dev, bool used_buffer, bool config_change)
{
    /* Set the reason of the irq.
       bit 0: used buffer
       bit 1: configuration change */
    dev->regs.InterruptStatus = 0;
    if (used_buffer) {
        dev->regs.InterruptStatus |= 0x1;
    }
    if (config_change) {
        dev->regs.InterruptStatus |= 0x2;
    }

    if (dev->transport_type == VIRTIO_TRANSPORT_PCI) {
        /*
         * virtIO spec 4.1.4.5.1 Device Requirements: ISR status capability
         */
        assert(pci_device_set_irq_status(dev->transport.pci.pci_handle, dev->regs.InterruptStatus != 0));
    }
}
