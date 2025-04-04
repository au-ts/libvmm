/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdbool.h>
#include <stdint.h>
#include <microkit.h>

#define VMM_MAGIC_LEN 3
static char VMM_MAGIC[VMM_MAGIC_LEN] = { 'v', 'm', 'm' };

#define VMM_MAX_IRQS 32
#define VMM_MAX_VCPUS 32
#define VMM_MAX_UIOS 16
#define MAX_VIRTIO_MMIO_DEVICES_PER_TYPE 4

typedef struct vmm_config_irq {
    uint8_t id;
    uint32_t irq;
} vmm_config_irq_t;

typedef struct vmm_config_virtio_mmio_device_regs {
    uint64_t base;
    uint32_t size;
    uint32_t irq;
} vmm_config_virtio_mmio_device_regs_t;

typedef struct vmm_config_virtio_console_device {
    vmm_config_virtio_mmio_device_regs_t regs;
} vmm_config_virtio_console_device_t;

typedef struct vmm_config_virtio_block_device {
    vmm_config_virtio_mmio_device_regs_t regs;
} vmm_config_virtio_block_device_t;

typedef struct vmm_config_virtio_net_device {
    vmm_config_virtio_mmio_device_regs_t regs;
} vmm_config_virtio_net_device_t;

typedef struct vmm_config_virtio_socket_device {
    vmm_config_virtio_mmio_device_regs_t regs;
    uint32_t cid;
    uint32_t shared_buffer_size;
    uintptr_t buffer_our;
    uintptr_t buffer_peer;
    uint32_t peer_ch;
} vmm_config_virtio_socket_device_t;

typedef struct vmm_config_virtio_sound_device {
    vmm_config_virtio_mmio_device_regs_t regs;
} vmm_config_virtio_sound_device_t;

typedef struct vmm_config_vcpu {
    uint8_t id;
} vmm_config_vcpu_t;

#define VMM_UIO_NAME_MAX_LEN 32
typedef struct vmm_config_uio_region {
    char name[VMM_UIO_NAME_MAX_LEN];
    uintptr_t guest_paddr;
    uintptr_t vmm_vaddr;
    uint64_t size;
    uint32_t irq;
} vmm_config_uio_region_t;

typedef struct vmm_config {
    char magic[VMM_MAGIC_LEN];
    uint64_t ram;
    uint64_t ram_size;
    uint64_t dtb;
    uint64_t initrd;
    uint8_t num_irqs;
    vmm_config_irq_t irqs[VMM_MAX_IRQS];
    uint8_t num_vcpus;
    vmm_config_vcpu_t vcpus[VMM_MAX_VCPUS];

    uint8_t num_virtio_mmio_console_devices;
    vmm_config_virtio_console_device_t virtio_mmio_console_devices[MAX_VIRTIO_MMIO_DEVICES_PER_TYPE];

    uint8_t num_virtio_mmio_block_devices;
    vmm_config_virtio_block_device_t virtio_mmio_block_devices[MAX_VIRTIO_MMIO_DEVICES_PER_TYPE];

    uint8_t num_virtio_mmio_net_devices;
    vmm_config_virtio_net_device_t virtio_mmio_net_devices[MAX_VIRTIO_MMIO_DEVICES_PER_TYPE];

    uint8_t num_virtio_mmio_socket_devices;
    vmm_config_virtio_socket_device_t virtio_mmio_socket_devices[MAX_VIRTIO_MMIO_DEVICES_PER_TYPE];

    uint8_t num_virtio_mmio_sound_devices;
    vmm_config_virtio_sound_device_t virtio_mmio_sound_devices[MAX_VIRTIO_MMIO_DEVICES_PER_TYPE];

    uint8_t num_uio_regions;
    vmm_config_uio_region_t uios[VMM_MAX_UIOS];
} vmm_config_t;

int vmm_config_irq_from_id(vmm_config_t *config, uint8_t id)
{
    for (int i = 0; i < config->num_irqs; i++) {
        if (config->irqs[i].id == id) {
            return config->irqs[i].irq;
        }
    }

    return -1;
}

static bool vmm_config_check_magic(void *config)
{
    char *magic = (char *)config;
    for (int i = 0; i < VMM_MAGIC_LEN; i++) {
        if (magic[i] != VMM_MAGIC[i]) {
            return false;
        }
    }

    return true;
}
