/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#define VIRTIO_GPU_UIO_IRQ 50

#define VIRTIO_GPU_UIO_CH 56 // @ericc: Put somewhere else in the future, probably should not be here

void virtio_gpu_uio_notified(sel4cp_channel ch);

void virtio_gpu_uio_ack(uint64_t vcpu_id, int irq, void *cookie);