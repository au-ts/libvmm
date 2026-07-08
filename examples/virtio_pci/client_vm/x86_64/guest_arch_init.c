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
#include <sddf/timer/config.h>

// @billn remove all serial stuff once virtio console works

/* Device slot of the virtio console device on bus 0.
 * Host bridge = 0, ISA bridge = 1 so we must avoid these.
 * Then on Intel, the integrated graphics is conventionally on slot 2 as well...
 * Make sure these two matches what is written in DSDT
 */
#define VIRTIO_CONSOLE_PCI_DEVICE_SLOT 3
#define VIRTIO_CONSOLE_PCI_IOAPIC_PIN 9

#define VIRTIO_NET_PCI_DEVICE_SLOT 4
#define VIRTIO_NET_PCI_IOAPIC_PIN 10

#define VIRTIO_BLK_PCI_DEVICE_SLOT 5
#define VIRTIO_BLK_PCI_IOAPIC_PIN 11

/* Data from sdfgen */
__attribute__((__section__(".timer_client_config"))) timer_client_config_t timer_config;
extern serial_client_config_t serial_config;
extern blk_client_config_t blk_config;
extern net_client_config_t net_config;
extern vmm_config_t vmm_config;

/* Data for the guest's kernel image. */
extern char _guest_kernel_image[];
extern char _guest_kernel_image_end[];
/* Data for the guest's ACPI Differentiated System Description Table (DSDT). */
extern char _guest_dsdt_aml[];
extern char _guest_dsdt_aml_end[];
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

static linux_x86_setup_ret_t linux_setup;
static seL4_VCPUContext initial_regs;

#define GUEST_CMDLINE "debug console=hvc0 loglevel=8"
#define GUEST_RAM_SIZE 0x10000000
#define GUEST_RAM_HVA 0x20000000

/* Matches DSDT */
#define PCI_MMIO_APERATURE_GPA 0x40000000
#define PCI_MMIO_APERATURE_SIZE 0x80000

bool guest_arch_init(void)
{
    assert(timer_config_check_magic(&timer_config));

    arch_guest_init_t args = {
        .bsp = true,
        .timer_ch = timer_config.driver_id,
        .num_guest_ram_regions = 1,
        .guest_ram_regions = { (struct guest_ram_region) {
            .gpa_start = LOW_RAM_START_GPA, .size = GUEST_RAM_SIZE, .vmm_vaddr = (void *)GUEST_RAM_HVA },
        },
        .pci_init = (struct guest_pci_init) {
                                   .ecam_gpa = 0,
                                   .ecam_size = 0,
                                   .mmio_aperature_gpa = PCI_MMIO_APERATURE_GPA,
                                   .mmio_aperature_size = PCI_MMIO_APERATURE_SIZE,
                               },
    };

    if (!guest_init(args)) {
        LOG_VMM_ERR("Failed to initialise VMM\n");
        return false;
    }

    /* Place all the binaries in the right locations before starting the guest */
    size_t kernel_size = _guest_kernel_image_end - _guest_kernel_image;
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;
    size_t dsdt_aml_size = _guest_dsdt_aml_end - _guest_dsdt_aml;

    if (!dsdt_aml_size) {
        LOG_VMM_ERR("DSDT AML image is empty\n");
        return false;
    }

    if (!linux_setup_images((uintptr_t)_guest_kernel_image, kernel_size, (uintptr_t)_guest_initrd_image, initrd_size,
                            _guest_dsdt_aml, dsdt_aml_size, GUEST_CMDLINE, &initial_regs, &linux_setup)) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return false;
    }

    return true;
}

bool virtio_arch_init(void)
{
    if (!virtio_pci_console_init(&virtio_console, 0, VIRTIO_CONSOLE_PCI_DEVICE_SLOT,
                                 X86_IOAPIC_IRQ_ROUTE(0, VIRTIO_CONSOLE_PCI_IOAPIC_PIN), &serial_rx_queue,
                                 &serial_tx_queue, serial_config.tx.id, serial_config.rx.id)) {
        LOG_VMM_ERR("Failed to initialise virtIO PCI Console device\n");
        return false;
    }

    blk_storage_info_t *storage_info = blk_config.virt.storage_info.vaddr;
    if (!virtio_pci_blk_init(&virtio_blk, 0, VIRTIO_BLK_PCI_DEVICE_SLOT,
                             X86_IOAPIC_IRQ_ROUTE(0, VIRTIO_BLK_PCI_IOAPIC_PIN), (uintptr_t)blk_config.data.vaddr,
                             blk_config.data.size, storage_info, &blk_queue, blk_config.virt.num_buffers,
                             blk_config.virt.id)) {
        LOG_VMM_ERR("Failed to initialise virtIO PCI Block device\n");
        return false;
    }

    if (!virtio_pci_net_init(&virtio_net, 0, VIRTIO_NET_PCI_DEVICE_SLOT,
                             X86_IOAPIC_IRQ_ROUTE(0, VIRTIO_NET_PCI_IOAPIC_PIN), &net_rx_queue, &net_tx_queue,
                             (uintptr_t)net_config.rx_data.vaddr, (uintptr_t)net_config.tx_data.vaddr, net_config.rx.id,
                             net_config.tx.id, net_config.mac_addr.addr)) {
        LOG_VMM_ERR("Failed to initialise virtIO PCI Network device\n");
        return false;
    }

    return true;
}

bool guest_arch_start(void)
{
    return guest_start_long_mode(linux_setup.kernel_entry_gpa, linux_setup.pml4_gpa, linux_setup.gdt_gpa,
                                 linux_setup.gdt_limit, &initial_regs);
}