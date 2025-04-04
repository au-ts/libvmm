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
#include <libvmm/arch/aarch64/linux.h>
#include <libvmm/arch/aarch64/fault.h>
#include <sddf/serial/queue.h>
#include <sddf/serial/config.h>
#include <sddf/util/printf.h>

__attribute__((__section__(".serial_client_config"))) serial_client_config_t serial_config;
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

/* Virtio socket */
static struct virtio_vsock_device virtio_vsock;

void init(void)
{
    assert(serial_config_check_magic(&serial_config));
    assert(vmm_config_check_magic(&vmm_config));

    /* Initialise the VMM and the VCPU */
    LOG_VMM("starting \"%s\"\n", microkit_name);

    // if (microkit_name[10] == '1') {
    //     return;
    // }

    /* Place all the binaries in the right locations before starting the guest */
    size_t kernel_size = _guest_kernel_image_end - _guest_kernel_image;
    size_t dtb_size = _guest_dtb_image_end - _guest_dtb_image;
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;
    uintptr_t kernel_pc = linux_setup_images(vmm_config.ram,
                                             (uintptr_t) _guest_kernel_image,
                                             kernel_size,
                                             (uintptr_t) _guest_dtb_image,
                                             vmm_config.dtb,
                                             dtb_size,
                                             (uintptr_t) _guest_initrd_image,
                                             vmm_config.initrd,
                                             initrd_size
                                            );
    if (!kernel_pc) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }

    /* Initialise the virtual GIC driver */
    bool success = virq_controller_init(GUEST_VCPU_ID);
    if (!success) {
        LOG_VMM_ERR("Failed to initialise emulated interrupt controller\n");
        return;
    }

    /* Initialise sDDF serial queues */
    serial_queue_init(&serial_rx_queue, serial_config.rx.queue.vaddr, serial_config.rx.data.size,
                      serial_config.rx.data.vaddr);
    serial_queue_init(&serial_tx_queue, serial_config.tx.queue.vaddr, serial_config.tx.data.size,
                      serial_config.tx.data.vaddr);

    /* Fetch VirtIO console device details from sdfgen */
    assert(vmm_config.num_virtio_mmio_console_devices == 1);
    vmm_config_virtio_console_device_t *mmio_console_dev = &vmm_config.virtio_mmio_console_devices[0];

    /* Initialise virtIO console device */
    success = virtio_mmio_console_init(&virtio_console,
                                       mmio_console_dev->regs.base,
                                       mmio_console_dev->regs.size,
                                       mmio_console_dev->regs.irq,
                                       &serial_rx_queue, &serial_tx_queue,
                                       serial_config.tx.id);
    assert(success);

    /* Fetch VirtIO socket device details from sdfgen */
    assert(vmm_config.num_virtio_mmio_socket_devices == 1);
    vmm_config_virtio_socket_device_t *mmio_socket_dev = &vmm_config.virtio_mmio_socket_devices[0];

    /* Initialise virtIO socket device */
    success = virtio_mmio_vsock_init(&virtio_vsock,
                                     mmio_socket_dev->regs.base,
                                     mmio_socket_dev->regs.size,
                                     mmio_socket_dev->regs.irq,
                                     mmio_socket_dev->cid,
                                     mmio_socket_dev->shared_buffer_size,
                                     mmio_socket_dev->buffer_our,
                                     mmio_socket_dev->buffer_peer,
                                     mmio_socket_dev->peer_ch);
    assert(success);

    /* Finally start the guest */
    guest_start(GUEST_VCPU_ID, kernel_pc, vmm_config.dtb, vmm_config.initrd);
    LOG_VMM("%s is ready\n", microkit_name);
}

void notified(microkit_channel ch)
{
    if (ch == serial_config.rx.id) {
        virtio_console_handle_rx(&virtio_console);
    } else if (ch == serial_config.tx.id) {
        /* Nothing to do */
    } else if (ch == vmm_config.virtio_mmio_socket_devices[0].peer_ch) {
        virtio_vsock_handle_rx(&virtio_vsock);
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
