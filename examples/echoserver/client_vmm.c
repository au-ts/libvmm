/*
 * Copyright 2023, UNSW
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
#include <sddf/timer/client.h>
#include <sddf/timer/config.h>
#include <sddf/blk/queue.h>
#include <sddf/blk/config.h>
#include <sddf/network/queue.h>
#include <sddf/network/config.h>
#include <sddf/util/printf.h>
#include <sddf/benchmark/sel4bench.h>
#include <sddf/benchmark/config.h>

__attribute__((__section__(".serial_client_config"))) serial_client_config_t serial_config;
__attribute__((__section__(".timer_client_config"))) timer_client_config_t timer_config;
__attribute__((__section__(".net_client_config"))) net_client_config_t net_config;
__attribute__((__section__(".vmm_config"))) vmm_config_t vmm_config;
__attribute__((__section__(".benchmark_client_config"))) benchmark_client_config_t benchmark_config;

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
#define VIRTIO_CONSOLE_IRQ (74)
#define VIRTIO_CONSOLE_BASE (0x130000)
#define VIRTIO_CONSOLE_SIZE (0x1000)

serial_queue_handle_t serial_rx_queue;
serial_queue_handle_t serial_tx_queue;

static struct virtio_console_device virtio_console;

#define VIRTIO_NET_IRQ (76)
#define VIRTIO_NET_BASE (0x160000)
#define VIRTIO_NET_SIZE (0x1000)
static struct virtio_net_device virtio_net;

net_queue_handle_t net_rx_queue;
net_queue_handle_t net_tx_queue;

void init(void)
{
    assert(serial_config_check_magic(&serial_config));
    assert(vmm_config_check_magic(&vmm_config));

    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", microkit_name);
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

    for (int i = 0; i < vmm_config.num_irqs; i++) {
        bool success = virq_register_passthrough(vmm_config.vcpus[0].id, vmm_config.irqs[i].irq, vmm_config.irqs[i].id);
        /* Should not be any reason for this to fail */
        assert(success);
    }

    serial_queue_init(&serial_rx_queue, serial_config.rx.queue.vaddr, serial_config.rx.data.size, serial_config.rx.data.vaddr);
    serial_queue_init(&serial_tx_queue, serial_config.tx.queue.vaddr, serial_config.tx.data.size, serial_config.tx.data.vaddr);

    /* Initialise virtIO console device */
    success = virtio_mmio_console_init(&virtio_console,
                                       VIRTIO_CONSOLE_BASE,
                                       VIRTIO_CONSOLE_SIZE,
                                       VIRTIO_CONSOLE_IRQ,
                                       &serial_rx_queue, &serial_tx_queue,
                                       serial_config.tx.id);

    assert(success);

    /* net_queue_init(&net_rx_queue, net_config.rx.free_queue.vaddr, net_config.rx.active_queue.vaddr, */
    /*                net_config.rx.num_buffers); */
    /* net_queue_init(&net_tx_queue, net_config.tx.free_queue.vaddr, net_config.tx.active_queue.vaddr, */
    /*                net_config.tx.num_buffers); */
    /* net_buffers_init(&net_tx_queue, 0); */
    /* success = virtio_mmio_net_init(&virtio_net, */
    /*                                VIRTIO_NET_BASE, */
    /*                                VIRTIO_NET_SIZE, */
    /*                                VIRTIO_NET_IRQ, */
    /*                                &net_rx_queue, &net_tx_queue, */
    /*                                (uintptr_t)net_config.rx_data.vaddr, (uintptr_t)net_config.tx_data.vaddr, */
    /*                                net_config.rx.id, net_config.tx.id, */
    /*                                net_config.mac_addr */
    /*                             ); */
    /* assert(success); */

    /* Finally start the guest */
    guest_start(GUEST_VCPU_ID, kernel_pc, vmm_config.dtb, vmm_config.initrd);
    LOG_VMM("%s is ready\n", microkit_name);
}

void notified(microkit_channel ch)
{
    bool success = virq_handle_passthrough(ch);
    if (success) {
        return;
    } else if (ch == serial_config.rx.id) {
        virtio_console_handle_rx(&virtio_console);
    } else if (ch == serial_config.tx.id || ch == net_config.tx.id) {
        /* Nothing to do */
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
