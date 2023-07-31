/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#define VIRTIO_GPU_ADDRESS_START    0x140000
#define VIRTIO_GPU_ADDRESS_END      0x150000

#define VIRTIO_GPU_CH               55   // @ericc: Put somewhere else in the future, probably should not be here

#define CONTROL_QUEUE               0
#define CURSOR_QUEUE                1
#define VIRTIO_MMIO_GPU_NUM_VIRTQUEUE   2

void virtio_gpu_ack(uint64_t vcpu_id, int irq, void *cookie);

virtio_emul_handler_t *get_virtio_gpu_emul_handler(void);

void virtio_gpu_emul_init(void);
