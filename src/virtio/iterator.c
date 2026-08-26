/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/guest_ram.h>
#include <libvmm/virq.h>
#include <libvmm/virtio/virtq.h>
#include <libvmm/virtio/virtio.h>
#include <libvmm/virtio/iterator.h>

bool virtio_desc_chain_iterator_new(virtio_queue_handler_t *vq_handler, uint16_t desc_head,
                                    virtio_desc_chain_iterator_t *ret)
{
    struct virtq *virtq = &vq_handler->virtq;
    struct virtq_desc *desc_ring = virtio_get_desc_ring(virtq);

    if (desc_head >= virtq->num) {
        LOG_VMM_ERR("descriptor head %u is out of bound %u.\n", desc_head, virtq->num);
        return false;
    }
    if (!desc_ring[desc_head].len) {
        LOG_VMM_ERR("descriptor head %u have zero size buffer.\n", desc_head);
        return false;
    }

    ret->vq_handler = vq_handler;
    ret->curr_desc = desc_head;
    ret->terminated = false;
    ret->steps = 0;

    if (desc_ring[desc_head].flags & VIRTQ_DESC_F_INDIRECT) {
        ret->in_indrect = true;
        ret->curr_indirect_pos = 0;
        ret->indirect_steps = 0;

        size_t indirect_table_size = desc_ring[desc_head].len;
        if (!indirect_table_size || indirect_table_size % sizeof(struct virtq_desc)) {
            LOG_VMM_ERR("bad indirect table length %zu at desc %u\n", indirect_table_size, desc_head);
            return false;
        }
        ret->indirect_table_entries = indirect_table_size / sizeof(struct virtq_desc);
    } else {
        ret->in_indrect = false;
    }

    return true;
}

virtio_desc_chain_iterator_status_t virtio_desc_chain_iterator_next(virtio_desc_chain_iterator_t *iter, uint64_t *gpa,
                                                                    size_t *len)
{
    if (iter->terminated) {
        return VIRTIO_ITERATOR_EXHAUSTED;
    }

    struct virtq *virtq = &iter->vq_handler->virtq;
    struct virtq_desc *desc_ring = virtio_get_desc_ring(virtq);

    if (iter->in_indrect) {
        if (!desc_ring[iter->curr_desc].len) {
            LOG_VMM_ERR("descriptor %u have zero size buffer.\n", iter->curr_desc);
            return VIRTIO_ITERATOR_ERROR;
        }
        if (iter->indirect_steps > iter->indirect_table_entries) {
            LOG_VMM_ERR("indirect table at desc %u is longer than expected or have loop\n", iter->curr_desc);
            return VIRTIO_ITERATOR_ERROR;
        }
        if (iter->curr_indirect_pos >= iter->indirect_table_entries) {
            LOG_VMM_ERR("bad next index %zu for indirect table at desc %u\n", iter->curr_indirect_pos, iter->curr_desc);
            return VIRTIO_ITERATOR_ERROR;
        }

        struct virtq_desc *indirect_descs = gpa_to_hva(desc_ring[iter->curr_desc].addr,
                                                       sizeof(struct virtq_desc) * iter->indirect_table_entries);
        if (!indirect_descs) {
            LOG_VMM_ERR("bad indirect table GPA 0x%lx at desc %u\n", desc_ring[iter->curr_desc].addr, iter->curr_desc);
            return VIRTIO_ITERATOR_ERROR;
        }
        if (indirect_descs[iter->curr_indirect_pos].flags & VIRTQ_DESC_F_INDIRECT) {
            LOG_VMM_ERR("nested indirect table at desc %u\n", iter->curr_desc);
            return VIRTIO_ITERATOR_ERROR;
        }

        *gpa = indirect_descs[iter->curr_indirect_pos].addr;
        *len = indirect_descs[iter->curr_indirect_pos].len;
        iter->indirect_steps += 1;

        if (indirect_descs[iter->curr_indirect_pos].flags & VIRTQ_DESC_F_NEXT) {
            iter->curr_indirect_pos = indirect_descs[iter->curr_indirect_pos].next;
        } else {
            /* An indirect table is always the last descriptor. */
            iter->terminated = true;
        }

    } else {
        if (iter->curr_desc >= virtq->num) {
            LOG_VMM_ERR("descriptor %u is out of bound %u.\n", iter->curr_desc, virtq->num);
            return VIRTIO_ITERATOR_ERROR;
        }
        if (iter->steps > virtq->num) {
            LOG_VMM_ERR("loop in descriptor %u.\n", iter->curr_desc);
            return VIRTIO_ITERATOR_ERROR;
        }
        if (!desc_ring[iter->curr_desc].len) {
            LOG_VMM_ERR("descriptor %u have zero size buffer.\n", iter->curr_desc);
            return VIRTIO_ITERATOR_ERROR;
        }

        *gpa = desc_ring[iter->curr_desc].addr;
        *len = desc_ring[iter->curr_desc].len;

        if (!(desc_ring[iter->curr_desc].flags & VIRTQ_DESC_F_NEXT)) {
            iter->terminated = true;
        } else {
            iter->curr_desc = desc_ring[iter->curr_desc].next;
            iter->steps++;

            if (iter->curr_desc >= virtq->num) {
                LOG_VMM_ERR("next descriptor %u is out of bound %u.\n", iter->curr_desc, virtq->num);
                return VIRTIO_ITERATOR_ERROR;
            }

            if (desc_ring[iter->curr_desc].flags & VIRTQ_DESC_F_INDIRECT) {
                iter->in_indrect = true;
                iter->curr_indirect_pos = 0;
                iter->indirect_steps = 0;

                size_t indirect_table_size = desc_ring[iter->curr_desc].len;
                if (!indirect_table_size || indirect_table_size % sizeof(struct virtq_desc)) {
                    LOG_VMM_ERR("bad indirect table length %zu at desc %u\n", indirect_table_size, iter->curr_desc);
                    return VIRTIO_ITERATOR_ERROR;
                }
                iter->indirect_table_entries = indirect_table_size / sizeof(struct virtq_desc);
            }
        }
    }

    return VIRTIO_ITERATOR_MOVED;
}