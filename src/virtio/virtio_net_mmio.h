/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "virtio/virtio_mmio.h"

#define VSWITCH_CONN_CH_1  2

// the amount of virtqueues
// it is set to 2 because the backend currently don't support VIRTIO_NET_F_MQ and VIRTIO_NET_F_CTRL_VQ,
// we (@jade) might work on supporting them in the future
#define VIRTIO_NET_MMIO_NUM_VIRTQUEUE   2

void virtio_net_ack(uint64_t vcpu_id, int irq, void *cookie);

virtio_mmio_handler_t *get_virtio_net_mmio_handler(void);

int vswitch_rx(sel4cp_channel channel);

void virtio_net_mmio_init(uintptr_t tx_avail, uintptr_t tx_used, uintptr_t tx_shared_dma_vaddr,
                          uintptr_t rx_avail, uintptr_t rx_used, uintptr_t rx_shared_dma_vaddr);
