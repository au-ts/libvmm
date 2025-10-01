/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <microkit.h>
#include <libvmm/config.h>
#include <libvmm/guest.h>
#include <libvmm/virq.h>
#include <libvmm/util/util.h>
#include <libvmm/virtio/virtio.h>
#include <libvmm/virtio/console.h>
#include <libvmm/virtio/block.h>
#include <libvmm/virtio/net.h>
#include <libvmm/arch/aarch64/linux.h>
#include <libvmm/arch/aarch64/fault.h>
#include <sddf/serial/queue.h>
#include <sddf/serial/config.h>
#include <sddf/blk/queue.h>
#include <sddf/blk/config.h>
#include <sddf/network/queue.h>
#include <sddf/network/config.h>
#include <sddf/util/printf.h>

__attribute__((__section__(".serial_client_config"))) serial_client_config_t serial_config;
__attribute__((__section__(".blk_client_config"))) blk_client_config_t blk_config;
__attribute__((__section__(".net_client_config"))) net_client_config_t net_config;
__attribute__((__section__(".vmm_config"))) vmm_config_t vmm_config;

/* Data for the guest's kernel image. */
extern char _guest_kernel_image[];
extern char _guest_kernel_image_end[];
/* Data for the device tree to be passed to the kernel. */
extern char _guest_dtb_image[];
extern char _guest_dtb_image_end[];
/* Data for the initial RAM disk to be passed to the kernel. */
extern char _guest_initrd_image[];
extern char _guest_initrd_image_end[];
/* Microkit will set this variable to the start of the guest RAM memory region. */
uintptr_t guest_ram_vaddr;

/* Virtio Console */
serial_queue_handle_t serial_rx_queue;
serial_queue_handle_t serial_tx_queue;

static struct virtio_console_device virtio_console;

/* /\* Virtio Block *\/ */
static blk_queue_handle_t blk_queue;
static struct virtio_blk_device virtio_blk;

/* /\* Virtio Net *\/ */
net_queue_handle_t net_rx_queue;
net_queue_handle_t net_tx_queue;
static struct virtio_net_device virtio_net;

/* PCI Configuration */
uintptr_t pci_ecam;
uintptr_t pci_memory_resource;

void init(void)
{
    assert(serial_config_check_magic(&serial_config));
    assert(blk_config_check_magic(&blk_config));
    assert(vmm_config_check_magic(&vmm_config));
    assert(net_config_check_magic(&net_config));

    blk_queue_init(&blk_queue, blk_config.virt.req_queue.vaddr, blk_config.virt.resp_queue.vaddr,
                   blk_config.virt.num_buffers);
    /* Want to print out configuration information, so wait until the config is ready. */
    blk_storage_info_t *storage_info = blk_config.virt.storage_info.vaddr;

    /* Busy wait until blk device is ready */
    while (!blk_storage_is_ready(storage_info));

    /* Initialise the VMM and the VCPU */
    LOG_VMM("starting \"%s\"\n", microkit_name);
    /* Place all the binaries in the right locations before starting the guest */
    size_t kernel_size = _guest_kernel_image_end - _guest_kernel_image;
    size_t dtb_size = _guest_dtb_image_end - _guest_dtb_image;
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;
    uintptr_t kernel_pc = linux_setup_images(vmm_config.ram, (uintptr_t)_guest_kernel_image, kernel_size,
                                             (uintptr_t)_guest_dtb_image, vmm_config.dtb, dtb_size,
                                             (uintptr_t)_guest_initrd_image, vmm_config.initrd, initrd_size);
    if (!kernel_pc) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }

    /* Initialise the virtual GIC driver */
    bool success = virq_controller_init();
    if (!success) {
        LOG_VMM_ERR("Failed to initialise emulated interrupt controller\n");
        return;
    }

    success = virtio_pci_ecam_init(0x10000000, 0x100000, 0x100000);
    assert(success);
    success = virtio_pci_register_memory_resource(0x20100000, 0x20100000, 0xFF00000);
    assert(success);

    serial_queue_init(&serial_rx_queue, serial_config.rx.queue.vaddr, serial_config.rx.data.size,
                      serial_config.rx.data.vaddr);
    serial_queue_init(&serial_tx_queue, serial_config.tx.queue.vaddr, serial_config.tx.data.size,
                      serial_config.tx.data.vaddr);

    success = virtio_pci_console_init(&virtio_console, 0, 48, &serial_rx_queue, &serial_tx_queue, serial_config.tx.id);
    assert(success);

    success = virtio_pci_blk_init(&virtio_blk, 1, 49, (uintptr_t)blk_config.data.vaddr, blk_config.data.size,
                                  storage_info, &blk_queue, blk_config.virt.num_buffers, blk_config.virt.id);
    assert(success);

    /* Initialise virtIO net device */
    net_queue_init(&net_rx_queue, net_config.rx.free_queue.vaddr, net_config.rx.active_queue.vaddr,
                   net_config.rx.num_buffers);
    net_queue_init(&net_tx_queue, net_config.tx.free_queue.vaddr, net_config.tx.active_queue.vaddr,
                   net_config.tx.num_buffers);
    net_buffers_init(&net_tx_queue, 0);

    success = virtio_pci_net_init(&virtio_net, 2, 50, &net_rx_queue, &net_tx_queue, (uintptr_t)net_config.rx_data.vaddr,
                                  (uintptr_t)net_config.tx_data.vaddr, net_config.rx.id, net_config.tx.id,
                                  net_config.mac_addr);
    assert(success);

    /* Finally start the guest */
    guest_start(kernel_pc, vmm_config.dtb, vmm_config.initrd);
    LOG_VMM("%s is ready\n", microkit_name);
}

void notified(microkit_channel ch)
{
    if (ch == serial_config.rx.id) {
        virtio_console_handle_rx(&virtio_console);
    } else if (ch == serial_config.tx.id || ch == net_config.tx.id) {
        /* Nothing to do */
    } else if (ch == blk_config.virt.id) {
        virtio_blk_handle_resp(&virtio_blk);
    } else if (ch == net_config.rx.id) {
        virtio_net_handle_rx(&virtio_net);
    } else {
        LOG_VMM_ERR("Unexpected channel, ch: 0x%lx\n", ch);
    }
}

seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo)
{
    bool success = fault_handle(child, msginfo);
    if (success) {
        /* Now that we have handled the fault successfully, we reply to it so
         * that the guest can resume execution. */
        *reply_msginfo = microkit_msginfo_new(0, 0);
        return seL4_True;
    }

    return seL4_False;
}
