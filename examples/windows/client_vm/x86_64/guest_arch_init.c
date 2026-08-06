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

#include <guest_arch_init.h>

/* Device slot of the virtio console device on bus 0.
 * Host bridge = 0, ISA bridge = 1 so we must avoid these.
 * Then on Intel, the integrated graphics is conventionally on slot 2 as well...
 * Make sure these two matches what is written in DSDT
 */
#define DISPLAY_PCI_DEVICE_SLOT 3

#define VIRTIO_NET_PCI_DEVICE_SLOT 4
#define VIRTIO_NET_PCI_IOAPIC_PIN 16

#define VIRTIO_BLK_PCI_DEVICE_SLOT 5
#define VIRTIO_BLK_PCI_IOAPIC_PIN 17

/* Data from sdfgen */
__attribute__((__section__(".timer_client_config"))) timer_client_config_t timer_config;
extern blk_client_config_t blk_config;
extern net_client_config_t net_config;
extern vmm_config_t vmm_config;

/* Data for the guest's ACPI Differentiated System Description Table (DSDT). */
extern char _guest_dsdt_aml[];
extern char _guest_dsdt_aml_end[];
/* Data for the guest's UEFI firmware. */
extern char _guest_firmware[];
extern char _guest_firmware_end[];

/* Bookkeeping structures for virtio devices */
extern struct virtio_blk_device virtio_blk;
extern struct virtio_net_device virtio_net;

/* sDDF data */
extern blk_queue_handle_t blk_queue;

extern net_queue_handle_t net_rx_queue;
extern net_queue_handle_t net_tx_queue;

/* 8GB RAM */
#define GUEST_LOW_RAM_SIZE 0xD0000000
#define GUEST_LOW_RAM_HVA 0x20000000
#define GUEST_HIGH_RAM_SIZE 0x120000000
#define GUEST_HIGH_RAM_HVA 0x100000000
#define GUEST_HIGH_RAM_GPA 0x100000000
#define GUEST_FLASH_GPA 0xFFA00000
#define GUEST_FLASH_SIZE 0x600000
#define GUEST_FLASH_HVA 0x10000000

#define COM1_IOAPIC_CHIP 0
#define COM1_IOAPIC_PIN 4
#define COM1_IO_PORT_ID 50
#define COM1_IO_PORT_ADDR 0x3F8
#define COM1_IO_PORT_SIZE 8

#define PS2_DATA_IO_PORT_ID 40
#define PS2_DATA_IO_PORT_ADDR 0x60
#define PS2_DATA_IO_PORT_SIZE 1
#define PS2_STS_IO_PORT_ID 41
#define PS2_STS_IO_PORT_ADDR 0x64
#define PS2_STS_IO_PORT_SIZE 1
#define PS2_FIRST_IRQ_IOAPIC_CHIP 0
#define PS2_FIRST_IRQ_IOAPIC_PIN 1
#define PS2_FIRST_IRQ_ID 42
#define PS2_SECOND_IRQ_IOAPIC_CHIP 0
#define PS2_SECOND_IRQ_IOAPIC_PIN 12
#define PS2_SECOND_IRQ_ID 43

/* Matches DSDT */
#define PCI_MMIO_APERATURE_GPA 0xE0000000
#define PCI_MMIO_APERATURE_SIZE 0x10000000

bool guest_arch_init(void)
{
    assert(timer_config_check_magic(&timer_config));

    arch_guest_init_t args = {
        .bsp = true,
        .timer_ch = timer_config.driver_id,
        .apicv_hva = 0x3000000000,
        .num_guest_ram_regions = 3,
        .guest_ram_regions = {
            (struct guest_ram_region) {
                .gpa_start = LOW_RAM_START_GPA, .size = GUEST_LOW_RAM_SIZE, .vmm_vaddr = (void *)GUEST_LOW_RAM_HVA
            },
            (struct guest_ram_region) {
                .gpa_start = GUEST_HIGH_RAM_GPA, .size = GUEST_HIGH_RAM_SIZE, .vmm_vaddr = (void *)GUEST_HIGH_RAM_HVA
            },
            (struct guest_ram_region) {
                .gpa_start = GUEST_FLASH_GPA, .size = GUEST_FLASH_SIZE, .vmm_vaddr = (void *)GUEST_FLASH_HVA
            },
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
    size_t dsdt_aml_size = _guest_dsdt_aml_end - _guest_dsdt_aml;
    size_t firmware_size = _guest_firmware_end - _guest_firmware;

    if (!dsdt_aml_size) {
        LOG_VMM_ERR("DSDT AML image is empty\n");
        return false;
    }
    if (!firmware_size) {
        LOG_VMM_ERR("Firmware image is empty\n");
        return false;
    }

    if (!uefi_setup_images((uintptr_t)_guest_firmware, firmware_size, (uintptr_t)_guest_dsdt_aml, dsdt_aml_size,
                           GUEST_FLASH_GPA, GUEST_FLASH_SIZE)) {
        LOG_VMM_ERR("Failed to initialise UEFI firmware\n");
        return false;
    }

    /* Pass through COM1 serial port */
    microkit_vcpu_x86_enable_ioport(GUEST_BOOT_VCPU_ID, COM1_IO_PORT_ID, COM1_IO_PORT_ADDR, COM1_IO_PORT_SIZE);

    /* Pass through serial IRQs */
    assert(virq_register_passthrough(X86_IOAPIC_IRQ_ROUTE(COM1_IOAPIC_CHIP, COM1_IOAPIC_PIN), SERIAL_IRQ_CH));
    microkit_irq_ack(SERIAL_IRQ_CH);

    /* Pass through QEMU Bochs display */
    assert(register_qemu_bochs_display_on_pci_bus(0, DISPLAY_PCI_DEVICE_SLOT, 0));

    /* Pass through PS2 keyboard and mouse */
    microkit_vcpu_x86_enable_ioport(GUEST_BOOT_VCPU_ID, PS2_DATA_IO_PORT_ID, PS2_DATA_IO_PORT_ADDR,
                                    PS2_DATA_IO_PORT_SIZE);
    microkit_vcpu_x86_enable_ioport(GUEST_BOOT_VCPU_ID, PS2_STS_IO_PORT_ID, PS2_STS_IO_PORT_ADDR, PS2_STS_IO_PORT_SIZE);
    assert(virq_register_passthrough(X86_IOAPIC_IRQ_ROUTE(PS2_FIRST_IRQ_IOAPIC_CHIP, PS2_FIRST_IRQ_IOAPIC_PIN),
                                     PS2_FIRST_IRQ_ID));
    microkit_irq_ack(PS2_FIRST_IRQ_ID);
    assert(virq_register_passthrough(X86_IOAPIC_IRQ_ROUTE(PS2_SECOND_IRQ_IOAPIC_CHIP, PS2_SECOND_IRQ_IOAPIC_PIN),
                                     PS2_SECOND_IRQ_ID));
    microkit_irq_ack(PS2_SECOND_IRQ_ID);

    return true;
}

bool virtio_arch_init(void)
{
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
    return guest_start_reset_state();
}