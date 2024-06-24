/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <microkit.h>
#include <util.h>
#include <vgic/vgic.h>
#include <linux.h>
#include <fault.h>
#include <guest.h>
#include <virq.h>
#include <tcb.h>
#include <vcpu.h>
#include <virtio/virtio.h>
#include <virtio/console.h>
#include <virtio/block.h>
#include <virtio/net.h>
#include <sddf/serial/queue.h>
#include <sddf/blk/queue.h>
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

serial_queue_handle_t serial_rx_h;
serial_queue_handle_t serial_tx_h;
sddf_handler_t sddf_serial_handlers[SDDF_SERIAL_NUM_HANDLES];

static struct virtio_device virtio_console;

/* Virtio Block */
#define BLK_CH 3

#define BLK_DATA_SIZE 0x200000

#define VIRTIO_BLK_IRQ (75)
#define VIRTIO_BLK_BASE (0x150000)
#define VIRTIO_BLK_SIZE (0x1000)

uintptr_t blk_req_queue;
uintptr_t blk_resp_queue;
uintptr_t blk_data;
uintptr_t blk_config;

blk_queue_handle_t blk_queue_h;
sddf_handler_t sddf_blk_handlers[SDDF_BLK_NUM_HANDLES];

static struct virtio_device virtio_blk;

/* Virtio Net VSwitch */
#define VSWITCH_TX_CH 4
#define VSWITCH_RX_CH 5

#define VSWITCH_IRQ (76)
#define VSWITCH_BASE (0x160000)
#define VSWITCH_SIZE (0x1000)

uintptr_t vswitch_rx_free;
uintptr_t vswitch_rx_active;
uintptr_t vswitch_tx_free;
uintptr_t vswitch_tx_active;

uintptr_t vswitch_rx_data_vaddr;
uintptr_t vswitch_rx_data_paddr;
uintptr_t vswitch_tx_data_vaddr;
uintptr_t vswitch_tx_data_paddr;

net_queue_handle_t vswitch_rx_h;
net_queue_handle_t vswitch_tx_h;
sddf_handler_t sddf_network_handlers[SDDF_NET_NUM_HANDLES];

static struct virtio_device virtio_vswitch;

void init(void)
{
    /* Busy wait until blk device is ready
       Need to put an empty assembly line to prevent compiler from optimising out the busy wait */
    // LOG_VMM("begin busy wait\n");
    // while (!((blk_storage_info_t *)blk_config)->ready) {
    //     asm("");
    // }
    // LOG_VMM("done busy waiting\n");

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

    /* virtIO console */
    sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].queue_h = &serial_rx_h;
    sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].config = NULL;
    sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].data = (uintptr_t)serial_rx_data;
    sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].data_size = 0; /* Unused */
    sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].ch = SERIAL_VIRT_RX_CH;

    sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].queue_h = &serial_tx_h;
    sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].config = NULL;
    sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].data = (uintptr_t)serial_tx_data;
    sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].data_size = 0; /* Unusued */
    sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].ch = SERIAL_VIRT_TX_CH;

    /* Initialise our sDDF queues for the serial device */
    serial_queue_init(sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].queue_h,
                      (serial_queue_t *)serial_rx_free,
                      (serial_queue_t *)serial_rx_active,
                      true,
                      NUM_ENTRIES,
                      NUM_ENTRIES);
    for (int i = 0; i < NUM_ENTRIES - 1; i++) {
        int ret = serial_enqueue_free(sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].queue_h, serial_rx_data + (i * BUFFER_SIZE),
                                      BUFFER_SIZE);
        if (ret != 0) {
            microkit_dbg_puts(microkit_name);
            microkit_dbg_puts(": server rx buffer population, unable to enqueue\n");
        }
    }
    /* Neither ring should be plugged and hence all buffers we send should actually end up at the driver. */
    assert(!serial_queue_plugged(((serial_queue_handle_t *)sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].queue_h)->free));
    assert(!serial_queue_plugged(((serial_queue_handle_t *)sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].queue_h)->active));

    serial_queue_init(sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].queue_h,
                      (serial_queue_t *)serial_tx_free,
                      (serial_queue_t *)serial_tx_active,
                      true,
                      NUM_ENTRIES,
                      NUM_ENTRIES);
    for (int i = 0; i < NUM_ENTRIES - 1; i++) {
        int ret = serial_enqueue_free(sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].queue_h, serial_tx_data + (i * BUFFER_SIZE),
                                      BUFFER_SIZE);
        assert(ret == 0);
        if (ret != 0) {
            microkit_dbg_puts(microkit_name);
            microkit_dbg_puts(": server tx buffer population, unable to enqueue\n");
        }
    }
    assert(!serial_queue_plugged(((serial_queue_handle_t *)sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].queue_h)->free));
    assert(!serial_queue_plugged(((serial_queue_handle_t *)sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].queue_h)->active));

    /* Initialise virtIO console device */
    success = virtio_mmio_device_init(&virtio_console, CONSOLE, VIRTIO_CONSOLE_BASE, VIRTIO_CONSOLE_SIZE,
                                      VIRTIO_CONSOLE_IRQ, sddf_serial_handlers);
    assert(success);

    /* virtIO vswitch */
    sddf_network_handlers[SDDF_NET_RX_HANDLE].queue_h = &vswitch_rx_h;
    sddf_network_handlers[SDDF_NET_RX_HANDLE].config = NULL;
    sddf_network_handlers[SDDF_NET_RX_HANDLE].data = vswitch_rx_data_vaddr;
    sddf_network_handlers[SDDF_NET_RX_HANDLE].data_size = 0; /* Unused */
    sddf_network_handlers[SDDF_NET_RX_HANDLE].ch = VSWITCH_RX_CH;

    sddf_network_handlers[SDDF_NET_TX_HANDLE].queue_h = &vswitch_tx_h;
    sddf_network_handlers[SDDF_NET_TX_HANDLE].config = NULL;
    sddf_network_handlers[SDDF_NET_TX_HANDLE].data = vswitch_tx_data_vaddr;
    sddf_network_handlers[SDDF_NET_TX_HANDLE].data_size = 0; /* Unusued */
    sddf_network_handlers[SDDF_NET_TX_HANDLE].ch = VSWITCH_TX_CH;

    microkit_dbg_puts("hello?\n");
    /* Initialise our sDDF queues for the serial device */

    net_queue_init(sddf_network_handlers[SDDF_NET_TX_HANDLE].queue_h,
                   (net_queue_t *)vswitch_tx_free,
                   (net_queue_t *)vswitch_tx_active, NUM_ENTRIES);

    net_buffers_init(sddf_network_handlers[SDDF_NET_TX_HANDLE].queue_h,
                     vswitch_tx_data_vaddr);

    microkit_dbg_puts("hello again\n");

    net_queue_init(sddf_network_handlers[SDDF_NET_RX_HANDLE].queue_h,
                   (net_queue_t *)vswitch_rx_free,
                   (net_queue_t *)vswitch_rx_active, NUM_ENTRIES);

    net_buffers_init(sddf_network_handlers[SDDF_NET_RX_HANDLE].queue_h,
                     vswitch_rx_data_vaddr);

    /* Initialise virtIO vswitch device */
    success = virtio_mmio_device_init(&virtio_vswitch, NET, VSWITCH_BASE, VSWITCH_SIZE,
                                      VSWITCH_IRQ, sddf_network_handlers);
    assert(success);

    /* virtIO block */
    sddf_blk_handlers[SDDF_BLK_DEFAULT_HANDLE].queue_h = &blk_queue_h;
    sddf_blk_handlers[SDDF_BLK_DEFAULT_HANDLE].config = (void *)blk_config;
    sddf_blk_handlers[SDDF_BLK_DEFAULT_HANDLE].data = (uintptr_t)blk_data;
    sddf_blk_handlers[SDDF_BLK_DEFAULT_HANDLE].data_size = BLK_DATA_SIZE;
    sddf_blk_handlers[SDDF_BLK_DEFAULT_HANDLE].ch = BLK_CH;

    /* Initialise our sDDF queues for the block device */
    blk_queue_init(sddf_blk_handlers[SDDF_BLK_DEFAULT_HANDLE].queue_h,
                   (blk_req_queue_t *)blk_req_queue,
                   (blk_resp_queue_t *)blk_resp_queue,
                   BLK_QUEUE_SIZE);

    /* Initialise virtIO block device */
    success = virtio_mmio_device_init(&virtio_blk, BLOCK, VIRTIO_BLK_BASE, VIRTIO_BLK_SIZE, VIRTIO_BLK_IRQ,
                                      sddf_blk_handlers);
    assert(success);

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
    case BLK_CH: {
        virtio_blk_handle_resp(&virtio_blk);
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
