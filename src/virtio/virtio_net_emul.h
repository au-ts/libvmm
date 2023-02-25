/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

// @jade: random numbers that I picked
#define VIRTIO_NET_ADDRESS_START    0x130000
#define VIRTIO_NET_ADDRESS_END      0x140000

// the amount of virtqueues
// it is set to 2 because the backend currently don't support VIRTIO_NET_F_MQ and VIRTIO_NET_F_CTRL_VQ,
// we (@jade) might work on supporting them in the future
#define VIRTIO_MMIO_NET_NUM_VIRTQUEUE   2

// we don't need ack as the guest OS should never send interrupts to us
void virtio_net_ack(uint64_t vcpu_id, int irq, void *cookie);

virtio_emul_handler_t *get_virtio_net_emul_handler(void);

void virtio_net_emul_init(void);
