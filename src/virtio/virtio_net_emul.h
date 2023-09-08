/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include "virtio_mmio.h"
#include "virtio_net_interface.h"

/**
 * Virtio_net device emulation based off of virtio mmio transport mode.
 */

// the amount of virtqueues
// it is set to 2 because the backend currently don't support VIRTIO_NET_F_MQ and VIRTIO_NET_F_CTRL_VQ,
// we (@jade) might work on supporting them in the future
#define VIRTIO_MMIO_NET_NUM_VIRTQUEUE   2

void virtio_net_ack(uint64_t vcpu_id, int irq, void *cookie);

typedef struct virtio_net_emul {
    virtio_mmio_emul_handler_t *mmio_handler;
    virtio_net_emul_interface_t *emul_interface;
    virtio_net_tt_interface_t *tt_interface;
    // @ericc: In future add virtio_net specific config space here
    // + other bookkeeping fields
} virtio_net_emul_t;

virtio_mmio_emul_handler_t *get_virtio_net_mmio_emul_handler(void);

virtio_net_emul_t *get_virtio_net_emul(void);

void virtio_net_emul_init(void);
