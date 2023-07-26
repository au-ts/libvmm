/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <sel4cp.h>
#include "../util/util.h"
#include "virtio_gpu_uio.h"
#include "include/config/virtio_gpu.h"
#include "../vgic/vgic.h"

#define VCPU_ID 0

uintptr_t uio_map0;

void virtio_gpu_uio_ack(uint64_t vcpu_id, int irq, void *cookie) {
    // printf("\"%s\"|VIRTIO GPU|INFO: virtio_gpu_uio_ack %d\n", sel4cp_name, irq);
}

void virtio_gpu_uio_notified(sel4cp_channel ch) {
    // printf("\"%s\"|VIRTIO GPU|INFO: virtio_gpu_uio_notified\n", sel4cp_name);
    bool success = vgic_inject_irq(VCPU_ID, VIRTIO_GPU_UIO_IRQ);
    // assert(success);
    printf(success ? "UIO interrupt success\n" : "UIO interrupt failure\n");
}

