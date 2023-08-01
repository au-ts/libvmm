/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

void gpu_virtqueue_to_sddf(uio_map_t *uio_map, virtqueue_t *vq);

void gpu_sddf_to_virtqueue(uio_map_t *uio_map, virtqueue_t *vq);