/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#define CONTROL_QUEUE               0
#define CURSOR_QUEUE                1
#define VIRTIO_MMIO_GPU_NUM_VIRTQUEUE   2

#define MAX_RESOURCE 64
#define MAX_MEM_ENTRIES 512

void virtio_gpu_mmio_ack(uint64_t vcpu_id, int irq, void *cookie);

virtio_mmio_handler_t *get_virtio_gpu_mmio_handler(void);

void virtio_gpu_mmio_init(uintptr_t gpu_tx_avail, uintptr_t gpu_tx_used, uintptr_t gpu_tx_shared_dma_vaddr,
                          uintptr_t gpu_rx_avail, uintptr_t gpu_rx_used, uintptr_t gpu_rx_shared_dma_vaddr);
