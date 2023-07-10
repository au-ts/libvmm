/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sel4cp.h>
#include "../util/printf.h"
#include "virtio_gpu_uio.h"
#include "include/config/virtio_gpu.h"

void virtio_gpu_notified(sel4cp_channel ch) {
    printf("\"%s\"|VIRTIO GPU|INFO: virtio_gpu_notified\n", sel4cp_name);
}

