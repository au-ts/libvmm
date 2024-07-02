/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <microkit.h>
#include <util.h>
#include <linux.h>
#include <fault.h>
#include <guest.h>
#include <virq.h>
#include <tcb.h>
#include <vcpu.h>
#include <virtio/virtio.h>
#include <virtio/console.h>
#include <virtio/net.h>
#include <sddf/serial/queue.h>
#include <sddf/network/queue.h>

#define GUEST_RAM_SIZE 0x6000000

#if defined(BOARD_qemu_arm_virt)
#define GUEST_DTB_VADDR 0x47f00000
#define GUEST_INIT_RAM_DISK_VADDR 0x47000000
#elif defined(BOARD_odroidc4)
#define GUEST_DTB_VADDR 0x25f10000
#define GUEST_INIT_RAM_DISK_VADDR 0x24000000
#else
#error Need to define guest kernel image address and DTB address
#endif

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
#define SERIAL_VIRT_TX_CH 1
#define SERIAL_VIRT_RX_CH 2

#define VIRTIO_CONSOLE_IRQ (74)
#define VIRTIO_CONSOLE_BASE (0x130000)
#define VIRTIO_CONSOLE_SIZE (0x1000)

uintptr_t serial_rx_free;
uintptr_t serial_rx_active;
uintptr_t serial_tx_free;
uintptr_t serial_tx_active;
uintptr_t serial_rx_data;
uintptr_t serial_tx_data;

static struct virtio_console_device virtio_console;

uintptr_t net_rx_free;
uintptr_t net_rx_active;
uintptr_t net_tx_free;
uintptr_t net_tx_active;
uintptr_t net_rx_data;
uintptr_t net_tx_data;

static struct virtio_net_device virtio_net;
#define NET_VIRT_RX_CH 3
#define NET_VIRT_TX_CH 4

#define VIRTIO_NET_IRQ (77)
#define VIRTIO_NET_BASE (0x190000)
#define VIRTIO_NET_SIZE (0x1000)

void init(void)
{
    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", microkit_name);
    /* Place all the binaries in the right locations before starting the guest */
    size_t kernel_size = _guest_kernel_image_end - _guest_kernel_image;
    size_t dtb_size = _guest_dtb_image_end - _guest_dtb_image;
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;
    uintptr_t kernel_pc = linux_setup_images(guest_ram_vaddr,
                                             (uintptr_t) _guest_kernel_image,
                                             kernel_size,
                                             (uintptr_t) _guest_dtb_image,
                                             GUEST_DTB_VADDR,
                                             dtb_size,
                                             (uintptr_t) _guest_initrd_image,
                                             GUEST_INIT_RAM_DISK_VADDR,
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

    /* Initialise our sDDF ring buffers for the serial device */
    serial_queue_handle_t rxq, txq;
    serial_queue_init(&rxq,
                      (serial_queue_t *)serial_rx_free,
                      (serial_queue_t *)serial_rx_active,
                      true,
                      NUM_ENTRIES,
                      NUM_ENTRIES);
    for (int i = 0; i < NUM_ENTRIES - 1; i++) {
        int ret = serial_enqueue_free(&rxq,
                               serial_rx_data + (i * BUFFER_SIZE),
                               BUFFER_SIZE);
        if (ret != 0) {
            microkit_dbg_puts(microkit_name);
            microkit_dbg_puts(": server rx buffer population, unable to enqueue buffer\n");
        }
    }
    serial_queue_init(&txq,
                      (serial_queue_t *)serial_tx_free,
                      (serial_queue_t *)serial_tx_active,
                      true,
                      NUM_ENTRIES,
                      NUM_ENTRIES);
    for (int i = 0; i < NUM_ENTRIES - 1; i++) {
        // Have to start at the memory region left of by the rx ring
        int ret = serial_enqueue_free(&txq,
                               serial_tx_data + ((i + NUM_ENTRIES) * BUFFER_SIZE),
                               BUFFER_SIZE);
        assert(ret == 0);
        if (ret != 0) {
            microkit_dbg_puts(microkit_name);
            microkit_dbg_puts(": server tx buffer population, unable to enqueue buffer\n");
        }
    }

    /* Neither ring should be plugged and hence all buffers we send should actually end up at the driver. */
    assert(!serial_queue_plugged(rxq.free));
    assert(!serial_queue_plugged(rxq.active));
    assert(!serial_queue_plugged(txq.free));
    assert(!serial_queue_plugged(txq.active));

    LOG_VMM("Initialised serial queues\n");

    /* Initialise virtIO console device */
    success = virtio_mmio_console_init(&virtio_console,
                                  VIRTIO_CONSOLE_BASE,
                                  VIRTIO_CONSOLE_SIZE,
                                  VIRTIO_CONSOLE_IRQ,
                                  &rxq, &txq,
                                  SERIAL_VIRT_TX_CH);
    
    LOG_VMM("Initialised virtio-console device\n");

    /* Initialise the virtIO net device */
    assert(net_rx_data != 0);
    assert(net_tx_data != 0);

    net_queue_handle_t rx, tx;
    net_queue_init(&rx, (net_queue_t *)net_rx_free, (net_queue_t *)net_rx_active, NUM_ENTRIES);
    net_queue_init(&tx, (net_queue_t *)net_tx_free, (net_queue_t *)net_tx_active, NUM_ENTRIES);

    LOG_VMM("Initialised net queues\n");

    net_buffers_init(&rx, net_rx_data);
    net_buffers_init(&tx, net_tx_data);

    LOG_VMM("Initialised net buffers\n");

    uint8_t mac[6] = {0x52, 0x54, 0x01, 0x00, 0x00, 0x16};
    success = virtio_mmio_net_init(&virtio_net,
                                   mac,
                                   VIRTIO_NET_BASE,
                                   VIRTIO_NET_SIZE,
                                   VIRTIO_NET_IRQ,
                                   &rx, &tx,
                                   net_rx_data, net_tx_data,
                                   NET_VIRT_RX_CH,
                                   NET_VIRT_TX_CH);
    assert(success);

    LOG_VMM("Initialised virtio-net device\n");

    /* Finally start the guest */
    guest_start(GUEST_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
}

void notified(microkit_channel ch)
{
    switch (ch) {
    case SERIAL_VIRT_RX_CH: {
        /* We have received an event from the serial multipelxor, so we
         * call the virtIO console handling */
        virtio_console_handle_rx(&virtio_console);
        break;
    }
    case NET_VIRT_RX_CH: {
        virtio_net_handle_rx(&virtio_net);
        break;
    }
    default:
        LOG_VMM_ERR("Unexpected channel, ch: 0x%lx\n", ch);
    }
}

/*
 * The primary purpose of the VMM after initialisation is to act as a fault-handler,
 * whenever our guest causes an exception, it gets delivered to this entry point for
 * the VMM to handle.
 */
void fault(microkit_id id, microkit_msginfo msginfo)
{
    bool success = fault_handle(id, msginfo);
    if (success) {
        /* Now that we have handled the fault successfully, we reply to it so
         * that the guest can resume execution. */
        microkit_fault_reply(microkit_msginfo_new(0, 0));
    }
}
