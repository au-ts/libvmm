/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include "../util/util.h"
#include "../vgic/vgic.h"
#include "virtio_mmio.h"
#include "virtio_gpu_emul.h"
#include "virtio_gpu_device.h"
#include "virtio_gpu_sddf.h"

static uio_map_t uio_map = {
    .addr = 0x30000000,
    .size = 0x40000
};

uio_map_t *get_uio_map() {
    return &uio_map;
}

void virtio_gpu_notified() {
    // Populate virtio stuff
    // vgic inject
}

// notify the guest that their avail buffer has been used
static void virtio_gpu_emul_handle_used_buffer_notif(struct virtio_emul_handler *self, uint16_t desc_head)
{
    // add to used ring
    struct vring *vring = &self->vqs[CONTROL_QUEUE].vring;

    struct vring_used_elem used_elem = {desc_head, 0};
    uint16_t guest_idx = vring->used->idx;

    vring->used->ring[guest_idx % vring->num] = used_elem;
    vring->used->idx++;
}

// // Tell guest that driver vm is done with buffers
//     bool success = vgic_inject_irq(VCPU_ID, VIRTIO_GPU_IRQ);
//     // we can't inject irqs?? panic.
//     assert(success);
