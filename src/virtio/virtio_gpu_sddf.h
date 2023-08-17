/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

int gpu_virtqueue_to_sddf(uio_map_t *uio_map, virtqueue_t *vq);

int gpu_sddf_to_virtqueue(virtqueue_t *vq, uio_map_t *uio_map);