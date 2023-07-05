/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stddef.h>
#include "virtio_mmio.h"
#include "virtio_gpu_emul.h"
#include "include/config/virtio_gpu.h"
#include "include/config/virtio_config.h"
#include "../vgic/vgic.h"

// virtio gpu mmio emul interface

// @jade, @ivanv: need to be able to get it from vgic
#define VCPU_ID 0

#define BIT_LOW(n) (1ul<<(n))
#define BIT_HIGH(n) (1ul<<(n - 32 ))

#define CONTROL_QUEUE 0
#define CURSOR_QUEUE 1

#define BUF_SIZE 0x1000

#define REG_RANGE(r0, r1)   r0 ... (r1 - 1)

// emul handler for an instance of virtio gpu
virtio_emul_handler_t gpu_emul_handler;

// the list of virtqueue handlers for an instance of virtio gpu
virtqueue_t gpu_vqs[VIRTIO_MMIO_GPU_NUM_VIRTQUEUE];

void virtio_gpu_ack(uint64_t vcpu_id, int irq, void *cookie) {
    // printf("\"%s\"|VIRTIO GPU|INFO: virtio_gpu_ack %d\n", sel4cp_name, irq);
    // nothing needs to be done here
}

virtio_emul_handler_t *get_virtio_gpu_emul_handler(void)
{
    // san check in case somebody wants to get the handler of an uninitialised backend
    if (gpu_emul_handler.data.VendorID != VIRTIO_MMIO_DEV_VENDOR_ID) {
        return NULL;
    }

    return &gpu_emul_handler;
}

static void virtio_gpu_emul_reset(virtio_emul_handler_t *self)
{
}

static int virtio_gpu_emul_get_device_features(virtio_emul_handler_t *self, uint32_t *features)
{
    if (self->data.Status & VIRTIO_CONFIG_S_FEATURES_OK) {
        printf("VIRTIO GPU|WARNING: driver somehow wants to read device features after FEATURES_OK\n");
    }

    switch (self->data.DeviceFeaturesSel) {
        // feature bits 0 to 31
        case 0:
            break;
        // features bits 32 to 63
        case 1:
            *features = BIT_HIGH(VIRTIO_F_VERSION_1);
            break;
        default:
            printf("VIRTIO GPU|INFO: driver sets DeviceFeaturesSel to 0x%x, which doesn't make sense\n", self->data.DeviceFeaturesSel);
            return 0;
    }
    return 1;
}

static int virtio_gpu_emul_set_driver_features(virtio_emul_handler_t *self, uint32_t features)
{
    int success = 1;

    switch (self->data.DriverFeaturesSel) {
        // feature bits 0 to 31
        case 0:
            // The device initialisation protocol says the driver should read device feature bits,
            // and write the subset of feature bits understood by the OS and driver to the device.
            break;

        // features bits 32 to 63
        case 1:
            success = (features == BIT_HIGH(VIRTIO_F_VERSION_1));
            break;

        default:
            printf("VIRTIO GPU|INFO: driver sets DriverFeaturesSel to 0x%x, which doesn't make sense\n", self->data.DriverFeaturesSel);
            success = 0;
    }
    if (success) {
        self->data.features_happy = 1;
    }
    return success;
}

static int virtio_gpu_emul_get_device_config(struct virtio_emul_handler *self, uint32_t offset, uint32_t *ret_val)
{
    return 1;
}

static int virtio_gpu_emul_set_device_config(struct virtio_emul_handler *self, uint32_t offset, uint32_t val)
{
    return 1;
}

static int virtio_gpu_emul_handle_queue_notify(struct virtio_emul_handler *self)
{
    return 1;
}

virtio_emul_funs_t gpu_emul_funs = {
    .device_reset = virtio_gpu_emul_reset,
    .get_device_features = virtio_gpu_emul_get_device_features,
    .set_driver_features = virtio_gpu_emul_set_driver_features,
    .get_device_config = virtio_gpu_emul_get_device_config,
    .set_device_config = virtio_gpu_emul_set_device_config,
    .queue_notify = virtio_gpu_emul_handle_queue_notify,
};

void virtio_gpu_emul_init(void)
{
    gpu_emul_handler.data.DeviceID = DEVICE_ID_VIRTIO_GPU;
    gpu_emul_handler.data.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    gpu_emul_handler.funs = &gpu_emul_funs;

    gpu_emul_handler.vqs = gpu_vqs;
}