/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#define VIRTIO_GPU_IRQ 73

#define VIRTIO_GPU_ADDRESS_START    0x140000
#define VIRTIO_GPU_ADDRESS_END      0x150000

// the amount of virtqueues
// 0: control q
// 1: cursor q
#define VIRTIO_MMIO_GPU_NUM_VIRTQUEUE   2

void virtio_gpu_ack(uint64_t vcpu_id, int irq, void *cookie);

virtio_emul_handler_t *get_virtio_gpu_emul_handler(void);

void virtio_gpu_emul_init(void);
