/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include "virtio_mmio.h"

#define VIRTIO_GPU_ADDRESS_START    0x140000
#define VIRTIO_GPU_ADDRESS_END      0x150000

#define VIRTIO_GPU_CH               55   // @ericc: Put somewhere else in the future, probably should not be here

#define CONTROL_QUEUE               0
#define CURSOR_QUEUE                1
#define VIRTIO_MMIO_GPU_NUM_VIRTQUEUE   2

#define MAX_RESOURCE 64
#define MAX_MEM_ENTRIES 512

// typedef struct virtio_gpu {
//     virtio_emul_handler_t handler;
//     // struct virtio_gpu_config config;
//     uint32_t *resource_ids;
//     struct virtio_gpu_mem_entry **mem_entries;
// } virtio_gpu_t;

void virtio_gpu_ack(uint64_t vcpu_id, int irq, void *cookie);

virtio_emul_handler_t *get_virtio_gpu_emul_handler(void);

void virtio_gpu_emul_init(void);
