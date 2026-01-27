/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <libvmm/guest.h>
#include <libvmm/config.h>
#include <libvmm/virq.h>
#include <libvmm/util/util.h>
#include <sddf/timer/client.h>
#include <sddf/blk/config.h>
#include <sddf/timer/config.h>
#include <sddf/serial/config.h>
#include <sddf/network/config.h>
#include <sddf/serial/queue.h>
#include <libvmm/virtio/console.h>
#include <libvmm/virtio/net.h>
#include <libvmm/virtio/block.h>

#include <libvmm/arch/x86_64/uefi.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/acpi.h>
#include <libvmm/arch/x86_64/hpet.h>
#include <libvmm/arch/x86_64/pci.h>
#include <libvmm/arch/x86_64/pit.h>
#include <libvmm/arch/x86_64/virq.h>
#include <libvmm/arch/x86_64/vcpu.h>

#include <sddf/util/custom_libc/string.h>

/* Virtio Console */
serial_queue_handle_t serial_rx_queue;
serial_queue_handle_t serial_tx_queue;
static struct virtio_console_device virtio_console;

/* Virtio Net */
net_queue_handle_t net_rx_queue;
net_queue_handle_t net_tx_queue;
static struct virtio_net_device virtio_net;

/* Virtio Block */
static blk_queue_handle_t blk_queue;
static struct virtio_blk_device virtio_blk;

// Device slot of the virtio console device on bus 0.
// Host bridge = 0, ISA bridge = 1 so we must avoid these.
// Then on Intel, the integrated graphics is conventionally on slot 2 as well...
// Make sure these two matches what is written in DSDT
#define VIRTIO_CONSOLE_PCI_DEVICE_SLOT 3
#define VIRTIO_CONSOLE_PCI_IOAPIC_PIN 15

#define VIRTIO_NET_PCI_DEVICE_SLOT 4
#define VIRTIO_NET_PCI_IOAPIC_PIN 14

#define VIRTIO_BLK_PCI_DEVICE_SLOT 5
#define VIRTIO_BLK_PCI_IOAPIC_PIN 13

// @billn sus, use package asm script
#include "board/x86_64_generic_vtx/simple_dsdt.hex"

__attribute__((__section__(".serial_client_config"))) serial_client_config_t serial_config;
__attribute__((__section__(".blk_client_config"))) blk_client_config_t blk_config;
__attribute__((__section__(".net_client_config"))) net_client_config_t net_config;
__attribute__((__section__(".vmm_config"))) vmm_config_t vmm_config;

uint64_t ps2_data_port_id = 22;
uint64_t ps2_data_port_addr = 0x60;
// see note in meta.py
uint64_t ps2_data_port_size = 4;

uint64_t ps2_sts_cmd_port_id = 23;
uint64_t ps2_sts_cmd_port_addr = 0x64;
uint64_t ps2_sts_cmd_port_size = 1;

#define FIRST_PS2_IRQ_CH 3
#define SECOND_PS2_IRQ_CH 4

/* Data for the guest's UEFI firmware image. */
extern char _guest_firmware_image[];
extern char _guest_firmware_image_end[];

uintptr_t guest_ram_vaddr = 0x30000000;
uint64_t guest_ram_size = 0x80000000; // 2 GiB
uintptr_t guest_flash_vaddr = 0x2000000;
uint64_t guest_flash_size = 0x600000;
uintptr_t guest_ecam_vaddr = 0x8000000;
uint64_t guest_ecam_size = 0x100000;

bool tsc_calibrating = true;
uint64_t tsc_pre, tsc_post, measured_tsc_hz;

void init(void)
{
    assert(serial_config_check_magic(&serial_config));
    assert(net_config_check_magic(&net_config));
    assert(blk_config_check_magic(&blk_config));

    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", microkit_name);

    serial_queue_init(&serial_rx_queue, serial_config.rx.queue.vaddr, serial_config.rx.data.size,
                      serial_config.rx.data.vaddr);
    serial_queue_init(&serial_tx_queue, serial_config.tx.queue.vaddr, serial_config.tx.data.size,
                      serial_config.tx.data.vaddr);

    blk_queue_init(&blk_queue, blk_config.virt.req_queue.vaddr, blk_config.virt.resp_queue.vaddr,
                   blk_config.virt.num_buffers);
    blk_storage_info_t *storage_info = blk_config.virt.storage_info.vaddr;
    /* Busy wait until blk device is ready */
    while (!blk_storage_is_ready(storage_info));



    size_t firm_size = _guest_firmware_image_end - _guest_firmware_image;
    assert(uefi_setup_images(guest_ram_vaddr, guest_ram_size, guest_flash_vaddr, guest_flash_size,
                             _guest_firmware_image, firm_size, simple_dsdt_aml_code, sizeof(simple_dsdt_aml_code)));

    vcpu_set_up_reset_state();

        // Set up the PCI bus
        // Make sure the backing MR is the same size as what we report to the guest via ACPI MCFG
    assert(guest_ecam_size == ECAM_SIZE);
    assert(virtio_pci_ecam_init(ECAM_GPA, guest_ecam_vaddr, guest_ecam_size));
    assert(virtio_pci_register_memory_resource(0xD0000000, 0x1000000, 0x200000));
    assert(pci_x86_init());

    microkit_vcpu_x86_enable_ioport(GUEST_BOOT_VCPU_ID, ps2_data_port_id, ps2_data_port_addr, ps2_data_port_size);
    microkit_vcpu_x86_enable_ioport(GUEST_BOOT_VCPU_ID, ps2_sts_cmd_port_id, ps2_sts_cmd_port_addr, ps2_sts_cmd_port_size);

    // assert(virtio_pci_console_init(&virtio_console, VIRTIO_CONSOLE_PCI_DEVICE_SLOT, VIRTIO_CONSOLE_PCI_IOAPIC_PIN,
    //                                &serial_rx_queue, &serial_tx_queue, serial_config.tx.id));

    net_queue_init(&net_rx_queue, net_config.rx.free_queue.vaddr, net_config.rx.active_queue.vaddr,
                   net_config.rx.num_buffers);
    net_queue_init(&net_tx_queue, net_config.tx.free_queue.vaddr, net_config.tx.active_queue.vaddr,
                   net_config.tx.num_buffers);
    net_buffers_init(&net_tx_queue, 0);

    {
        bool success = virtio_pci_net_init(&virtio_net, VIRTIO_NET_PCI_DEVICE_SLOT, VIRTIO_NET_PCI_IOAPIC_PIN,
                                           &net_rx_queue, &net_tx_queue, (uintptr_t)net_config.rx_data.vaddr,
                                           (uintptr_t)net_config.tx_data.vaddr, net_config.rx.id, net_config.tx.id,
                                           net_config.mac_addr);
        assert(success);
    }
    {
        bool success = virtio_pci_blk_init(&virtio_blk, VIRTIO_BLK_PCI_DEVICE_SLOT, VIRTIO_BLK_PCI_IOAPIC_PIN,
                                      (uintptr_t)blk_config.data.vaddr, blk_config.data.size, storage_info, &blk_queue,
                                      blk_config.virt.num_buffers, blk_config.virt.id);
        assert(success);
    }

    LOG_VMM("Measuring TSC frequency...\n");
    sddf_timer_set_timeout(TIMER_DRV_CH_FOR_LAPIC, NS_IN_S);
    tsc_pre = rdtsc();
}

void notified(microkit_channel ch)
{
    if (ch == serial_config.rx.id) {
        // virtio_console_handle_rx(&virtio_console);
        return;
    } else if (ch == net_config.rx.id) {
        virtio_net_handle_rx(&virtio_net);
        return;
    } else if (ch == net_config.tx.id || ch == serial_config.tx.id) {
        // Nothing to do.
        return;
    } else if (ch == blk_config.virt.id) {
        virtio_blk_handle_resp(&virtio_blk);
        return;
    }

    switch (ch) {
    case TIMER_DRV_CH_FOR_LAPIC: {
        if (tsc_calibrating) {
            tsc_post = rdtsc();
            measured_tsc_hz = tsc_post - tsc_pre;
            LOG_VMM("TSC frequency is %lu Hz\n", measured_tsc_hz);
            tsc_calibrating = false;

            /* Initialise the virtual APIC */
            bool success = virq_controller_init(measured_tsc_hz);
            if (!success) {
                LOG_VMM_ERR("Failed to initialise virtual IRQ controller\n");
                return;
            }

            microkit_irq_ack(FIRST_PS2_IRQ_CH);
            microkit_irq_ack(SECOND_PS2_IRQ_CH);

            /* Pass through PS2 IRQs */
            assert(virq_ioapic_register_passthrough(0, 1, FIRST_PS2_IRQ_CH));
            assert(virq_ioapic_register_passthrough(0, 12, SECOND_PS2_IRQ_CH));

            /* Start vCPU in *real* mode with paging off */
            guest_start(0xfff0, 0, 0);
        } else {
            handle_lapic_timer_nftn(GUEST_BOOT_VCPU_ID);
        }
        break;
    }
    case TIMER_DRV_CH_FOR_PIT:
        pit_handle_timer_ntfn();
        break;
    case TIMER_DRV_CH_FOR_HPET_CH0:
    case TIMER_DRV_CH_FOR_HPET_CH1:
    case TIMER_DRV_CH_FOR_HPET_CH2:
        hpet_handle_timer_ntfn(ch);
        break;
    default:
        virq_ioapic_handle_passthrough(ch);
    }
}

/*
 * The primary purpose of the VMM after initialisation is to act as a fault-handler.
 * Whenever our guest causes an exception, it gets delivered to this entry point for
 * the VMM to handle.
 */
// seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo)
// {
// #if defined(CONFIG_ARCH_AARCH64)
//     bool success = fault_handle(child, msginfo);
//     if (success) {
//         /* Now that we have handled the fault successfully, we reply to it so
//          * that the guest can resume execution. */
//         *reply_msginfo = microkit_msginfo_new(0, 0);
//         return seL4_True;
//     }
// #endif

//     return seL4_False;
// }
