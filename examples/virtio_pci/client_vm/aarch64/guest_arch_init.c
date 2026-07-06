/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stddef.h>
#include <stdint.h>
#include <microkit.h>
#include <libvmm/libvmm.h>
#include <sddf/serial/queue.h>
#include <sddf/serial/config.h>
#include <sddf/blk/queue.h>
#include <sddf/blk/config.h>
#include <sddf/network/queue.h>
#include <sddf/network/config.h>

/* These GPAs depends on what's defined in your guest DTB. */
#define GUEST_RAM_START_GPA 0x40000000
#define PCI_ECAM_GPA 0x10000000
#define PCI_ECAM_SIZE 0x100000
#define PCI_MMIO_APERATURE_GPA 0x20100000
#define PCI_MMIO_APERATURE_SIZE 0x0ff00000

/* Data from sdfgen */
extern serial_client_config_t serial_config;
extern blk_client_config_t blk_config;
extern net_client_config_t net_config;
extern vmm_config_t vmm_config;

/* Data for the guest's kernel image. */
extern char _guest_kernel_image[];
extern char _guest_kernel_image_end[];
/* Data for the device tree to be passed to the kernel. */
extern char _guest_dtb_image[];
extern char _guest_dtb_image_end[];
/* Data for the initial RAM disk to be passed to the kernel. */
extern char _guest_initrd_image[];
extern char _guest_initrd_image_end[];

/* Bookkeeping structures for virtio devices */
extern struct virtio_console_device virtio_console;
extern struct virtio_blk_device virtio_blk;
extern struct virtio_net_device virtio_net;

/* sDDF data */
extern serial_queue_handle_t serial_rx_queue;
extern serial_queue_handle_t serial_tx_queue;

extern blk_queue_handle_t blk_queue;

extern net_queue_handle_t net_rx_queue;
extern net_queue_handle_t net_tx_queue;

static uintptr_t kernel_pc = 0;

bool guest_arch_init(void)
{
    arch_guest_init_t args = { .num_vcpus = 1,
                               .num_guest_ram_regions = 1,
                               .guest_ram_regions = { (struct guest_ram_region) {
                                   .gpa_start = GUEST_RAM_START_GPA,
                                   .size = vmm_config.ram_size,
                                   .vmm_vaddr = (void *)vmm_config.ram } },
                               .pci_init = (struct guest_pci_init) {
                                   .ecam_gpa = PCI_ECAM_GPA,
                                   .ecam_size = PCI_ECAM_SIZE,
                                   .mmio_aperature_gpa = PCI_MMIO_APERATURE_GPA,
                                   .mmio_aperature_size = PCI_MMIO_APERATURE_SIZE,
                               } };

    if (!guest_init(args)) {
        LOG_VMM_ERR("Failed to initialise VMM\n");
        return false;
    }

    /* Place all the binaries in the right locations before starting the guest */
    size_t kernel_size = _guest_kernel_image_end - _guest_kernel_image;
    size_t dtb_size = _guest_dtb_image_end - _guest_dtb_image;
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;
    kernel_pc = linux_setup_images(GUEST_RAM_START_GPA, (uintptr_t)_guest_kernel_image, kernel_size,
                                   (uintptr_t)_guest_dtb_image, vmm_config.dtb, dtb_size,
                                   (uintptr_t)_guest_initrd_image, vmm_config.initrd, initrd_size);
    if (!kernel_pc) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return false;
    }

    return true;
}

bool virtio_arch_init(void)
{
    if (!virtio_pci_console_init(&virtio_console, 0, 0, ARM_GIC_IRQ_ROUTE(GUEST_BOOT_VCPU_ID, 48), &serial_rx_queue,
                                 &serial_tx_queue, serial_config.tx.id, serial_config.rx.id)) {
        LOG_VMM_ERR("Failed to initialise virtIO PCI Console device\n");
        return false;
    }

    blk_storage_info_t *storage_info = blk_config.virt.storage_info.vaddr;
    if (!virtio_pci_blk_init(&virtio_blk, 0, 1, ARM_GIC_IRQ_ROUTE(GUEST_BOOT_VCPU_ID, 49),
                             (uintptr_t)blk_config.data.vaddr, blk_config.data.size, storage_info, &blk_queue,
                             blk_config.virt.num_buffers, blk_config.virt.id)) {
        LOG_VMM_ERR("Failed to initialise virtIO PCI Block device\n");
        return false;
    }

    if (!virtio_pci_net_init(&virtio_net, 0, 2, ARM_GIC_IRQ_ROUTE(GUEST_BOOT_VCPU_ID, 50), &net_rx_queue, &net_tx_queue,
                             (uintptr_t)net_config.rx_data.vaddr, (uintptr_t)net_config.tx_data.vaddr, net_config.rx.id,
                             net_config.tx.id, net_config.mac_addr.addr)) {
        LOG_VMM_ERR("Failed to initialise virtIO PCI Network device\n");
        return false;
    }

    return true;
}

bool guest_arch_start(void)
{
    return guest_start(kernel_pc, vmm_config.dtb, vmm_config.initrd);
}