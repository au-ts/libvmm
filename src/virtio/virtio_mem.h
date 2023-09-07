/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

/* Random numbers that I picked, if it overlaps with other memory regions,
 * change this to use another slot. You might also need to change:
 * 1. VIRTIO_<device name>_ADDRESS_START
 * 2. VIRTIO_<device name>_ADDRESS_END
 * 3. The virtio device entry in your DTS file
 */
#define VIRTIO_ADDRESS_START    0x130000
#define VIRTIO_ADDRESS_END      0x150000

#define VIRTIO_NET_ADDRESS_START    0x130000
#define VIRTIO_NET_ADDRESS_END      0x140000

// #define VIRTIO_GPU_ADDRESS_START    0x140000
// #define VIRTIO_GPU_ADDRESS_END      0x150000