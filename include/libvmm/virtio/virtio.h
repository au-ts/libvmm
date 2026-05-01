/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <libvmm/virtio/mmio.h>
#include <libvmm/virtio/pci.h>

/*
 * Default maximum capacity of each virtIO queue. Currently applies to all
 * virtIO queues for all virtIO devices. In the future, this could be
 * configurable but our use-cases have not required it yet.
 */
#define VIRTIO_DEFAULT_QUEUE_SIZE 128

/*
 * All terminology used and functionality of the virtIO device implementation
 * adheres with the following specification:
 * Virtual I/O Device (VIRTIO) Version 1.2
 */

typedef enum virtio_transport_type {
    VIRTIO_TRANSPORT_PCI = 1,
    VIRTIO_TRANSPORT_MMIO = 2,
    VIRTIO_TRANSPORT_CCW = 3,
} virtio_transport_type_t;

typedef union virtio_transport_data {
    virtio_pci_data_t pci;
    virtio_mmio_data_t mmio;
} virtio_transport_data_t;

/* Everything needed at runtime for a virtIO device to function. */
typedef struct virtio_device {
    virtio_transport_type_t transport_type;
    virtio_transport_data_t transport;
    virtio_device_regs_t regs;
    virtio_device_funs_t *funs;
    /* List of virt queues for the device */
    virtio_queue_handler_t *vqs;
    /* Length of the vqs list */
    size_t num_vqs;
    /* Virtual IRQ associated with this virtIO device */
    size_t virq;
    /* Device specific data such as sDDF queues */
    void *device_data;
    /* True if we are happy with what the driver requires */
    bool features_happy;
} virtio_device_t;

static inline struct virtq *get_current_virtq_by_handler(virtio_device_t *dev)
{
    assert(dev->regs.QueueSel < dev->num_vqs);
    return &dev->vqs[dev->regs.QueueSel].virtq;
}

/*
 * Registers a new virtIO device at a given guest-physical region.
 *
 * Assumes the virtio_device_t *dev struct passed has been populated
 * and virtual IRQ associated with the device has been registered.
 */
bool virtio_mmio_register_device(virtio_device_t *dev, uintptr_t region_base, uintptr_t region_size, size_t virq);

/*
 * Given a descriptor head, walk the descriptor chain and compute the sum of the
 * length of all the desciptor chain's buffers
 */
uint64_t virtio_desc_chain_payload_len(virtio_queue_handler_t *vq_handler, uint16_t desc_head);

/*
 * Given a scatter gather list of a virtio request in the form of a descriptor
 * head, copy a chunk of data from the list, starting from `read_off` into `data`.
 *
 * For example, if you want the body of a virtio block read or write request,
 * `read_off` should be `sizeof(struct virtio_blk_outhdr)`.
 *
 * Return true if the operation completed successfully, false if the amount of data
 * overflows the scatter gather list or invalid descriptor chain.
 */
bool virtio_read_data_from_desc_chain(virtio_queue_handler_t *vq_handler, uint16_t desc_head, uint64_t bytes_to_read,
                                      uint64_t read_off, char *data);

/* Same idea as above but we are now writing to the scatter gather buffers in guest RAM */
bool virtio_write_data_to_desc_chain(virtio_queue_handler_t *vq_handler, uint16_t desc_head, uint64_t bytes_to_write,
                                     uint64_t write_off, char *data);

/* Peek an available descriptor head from a virtq */
bool virtio_virtq_peek_avail(virtio_queue_handler_t *vq_handler, uint16_t *ret);

/* Pop an available descriptor head from a virtq */
bool virtio_virtq_pop_avail(virtio_queue_handler_t *vq_handler, uint16_t *ret);

/*
 * Given a descriptor head and the number of bytes that were written to it by the VMM,
 * place it in the used queue
 */
void virtio_virtq_add_used(virtio_queue_handler_t *vq_handler, uint16_t desc_head, uint32_t bytes_written);
