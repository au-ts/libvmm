/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdint.h>
#include <microkit.h>

#define VMM_MAX_IRQS 32
#define VMM_MAX_VCPUS 32
#define VMM_MAX_VIRTIO_DEVICES 32

typedef struct vmm_config_irq {
    uint8_t id;
    uint32_t irq;
} vmm_config_irq_t;

typedef struct vmm_config_virtio_device {
    uint64_t base;
    uint64_t size;
    uint32_t irq;
} vmm_config_virtio_device_t;

typedef struct vmm_config_vcpu {
    uint8_t id;
} vmm_config_vcpu_t;

typedef struct vmm_config {
    uint64_t ram;
    uint64_t ram_size;
    uint64_t dtb;
    uint64_t initrd;
    uint8_t num_irqs;
    vmm_config_irq_t irqs[VMM_MAX_IRQS];
    uint8_t num_vcpus;
    vmm_config_vcpu_t vcpus[VMM_MAX_VCPUS];
    uint8_t num_virtio_devices;
    vmm_config_virtio_device_t virtio_devices[VMM_MAX_VIRTIO_DEVICES];
} vmm_config_t;

int vmm_config_irq_from_id(vmm_config_t *config, uint8_t id) {
    for (int i = 0; i < config->num_irqs; i++) {
        if (config->irqs[i].id == id) {
            return config->irqs[i].irq;
        }
    }

    return -1;
}
