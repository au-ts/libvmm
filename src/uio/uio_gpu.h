/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#define UIO_GPU_IRQ 50

#define UIO_GPU_CH 56 // @ericc: Put somewhere else in the future, probably should not be here

// #define VIRTIO_GPU_UIO_ADDRESS_START 0x30000000
// #define VIRTIO_GPU_UIO_ADDRESS_END 0x30020000

void uio_gpu_notified(sel4cp_channel ch);

void uio_gpu_ack(uint64_t vcpu_id, int irq, void *cookie);