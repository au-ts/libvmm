/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#define VIRTIO_GPU_IRQ 75

typedef struct uio_map {
    uintptr_t addr;
    uint64_t size;
} uio_map_t;

uio_map_t *get_uio_map();

void virtio_gpu_notified();