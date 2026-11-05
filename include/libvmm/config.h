/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <microkit.h>
#include <libvmm/guest_ram.h>

#define VMM_MAGIC_LEN 3
static char VMM_MAGIC[VMM_MAGIC_LEN] = { 'v', 'm', 'm' };

#define VMM_MAX_RAM_REGIONS GUEST_MAX_RAM_REGIONS
#define VMM_MAX_IRQS 32
#define VMM_MAX_VCPUS 32
#define VMM_MAX_UIOS 16
#define VMM_MAX_VIRTIO_MMIO_DEVICES 32
#define VMM_MAX_IOPORTS 16

typedef enum vmm_config_ram_kind {
    VMM_CONFIG_RAM = 0,
    VMM_CONFIG_FLASH = 1,
} vmm_config_ram_kind_t;

typedef struct vmm_config_ram_region {
    uint64_t gpa;
    uint64_t size;
    uintptr_t vmm_vaddr;
    uint8_t kind;
} vmm_config_ram_region_t;

typedef struct vmm_config_irq {
    uint8_t id;
    /* GIC INTID on ARM, guest I/O APIC pin on x86 */
    uint32_t irq;
} vmm_config_irq_t;

typedef struct vmm_config_virtio_mmio_device {
    uint8_t type;
    uint64_t base;
    uint32_t size;
    uint32_t irq;
} vmm_config_virtio_mmio_device_t;

typedef struct vmm_config_vcpu {
    uint8_t id;
} vmm_config_vcpu_t;

typedef struct vmm_config_ioport {
    uint8_t id;
    uint16_t addr;
    uint16_t size;
} vmm_config_ioport_t;

#define VMM_UIO_NAME_MAX_LEN 32
typedef struct vmm_config_uio_region {
    char name[VMM_UIO_NAME_MAX_LEN];
    uintptr_t guest_paddr;
    uintptr_t vmm_vaddr;
    uint64_t size;
    uint32_t irq;
} vmm_config_uio_region_t;

typedef struct vmm_config {
    char magic[VMM_MAGIC_LEN + 1];
    uint8_t num_ram_regions;
    vmm_config_ram_region_t ram_regions[VMM_MAX_RAM_REGIONS];
    /* Begin DTB guest only fields */
    uint64_t dtb;
    /* End DTB guest only fields */
    uint64_t initrd;
    uint8_t num_irqs;
    vmm_config_irq_t irqs[VMM_MAX_IRQS];
    uint8_t num_vcpus;
    vmm_config_vcpu_t vcpus[VMM_MAX_VCPUS];
    uint8_t num_virtio_mmio_devices;
    vmm_config_virtio_mmio_device_t virtio_mmio_devices[VMM_MAX_VIRTIO_MMIO_DEVICES];
    uint8_t num_uio_regions;
    vmm_config_uio_region_t uios[VMM_MAX_UIOS];
    /* Begin x86 guest only fields */
    uint8_t num_ioports;
    vmm_config_ioport_t ioports[VMM_MAX_IOPORTS];
    /* End x86 guest only fields */
} vmm_config_t;

static int vmm_config_irq_from_id(vmm_config_t *config, uint8_t id)
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

/* Fill guest_ram_regions in arch_guest_init_t straight from config.
 * Returns the number of regions written. */
static size_t vmm_config_guest_ram_regions(vmm_config_t *config, struct guest_ram_region *out)
{
    for (int i = 0; i < config->num_ram_regions; i++) {
        out[i] = (struct guest_ram_region) {
            .gpa_start = config->ram_regions[i].gpa,
            .size = config->ram_regions[i].size,
            .vmm_vaddr = (void *)config->ram_regions[i].vmm_vaddr,
        };
    }

    return config->num_ram_regions;
}

/* Returns the first region of the given kind, e.g. the UEFI flash. */
static vmm_config_ram_region_t *vmm_config_find_ram(vmm_config_t *config, vmm_config_ram_kind_t kind)
{
    for (int i = 0; i < config->num_ram_regions; i++) {
        if (config->ram_regions[i].kind == kind) {
            return &config->ram_regions[i];
        }
    }

    return NULL;
}