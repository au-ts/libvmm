/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <microkit.h>
#include "util/util.h"
#include "arch/aarch64/vgic/vgic.h"
#include "arch/aarch64/linux.h"
#include "arch/aarch64/fault.h"
#include "guest.h"
#include "virq.h"
#include "tcb.h"
#include "vcpu.h"
#include "virtio/virtio.h"
#include "virtio/console.h"
#include "virtio/block.h"
#include <sddf/serial/shared_ringbuffer.h>
#include <sddf/blk/shared_queue.h>

/*
 * As this is just an example, for simplicity we just make the size of the
 * guest's "RAM" the same for all platforms. For just booting Linux with a
 * simple user-space, 0x10000000 bytes (256MB) is plenty.
 */
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
#define SERIAL_MUX_TX_CH 1
#define SERIAL_MUX_RX_CH 2

#define VIRTIO_CONSOLE_IRQ (74)
#define VIRTIO_CONSOLE_BASE (0x130000)
#define VIRTIO_CONSOLE_SIZE (0x1000)

uintptr_t serial_rx_free;
uintptr_t serial_rx_used;
uintptr_t serial_tx_free;
uintptr_t serial_tx_used;
uintptr_t serial_rx_data;
uintptr_t serial_tx_data;

ring_handle_t serial_rx_h;
ring_handle_t serial_tx_h;
sddf_handler_t sddf_serial_handlers[SDDF_SERIAL_NUM_HANDLES];

static struct virtio_device virtio_console;

/* Virtio Block */
#define BLK_CH 3

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

void init(void) {
    // return;
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
    sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].ch = SERIAL_MUX_RX_CH;
    
    sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].queue_h = &serial_tx_h;
    sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].config = NULL;
    sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].data = (uintptr_t)serial_tx_data;
    sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].ch = SERIAL_MUX_TX_CH;

    /* Initialise our sDDF ring buffers for the serial device */
    ring_init(sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].queue_h,
            (ring_buffer_t *)serial_rx_free,
            (ring_buffer_t *)serial_rx_used,
            true,
            NUM_BUFFERS,
            NUM_BUFFERS);
    for (int i = 0; i < NUM_BUFFERS - 1; i++) {
        int ret = enqueue_free(sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].queue_h, serial_rx_data + (i * BUFFER_SIZE), BUFFER_SIZE, NULL);
        if (ret != 0) {
            microkit_dbg_puts(microkit_name);
            microkit_dbg_puts(": server rx buffer population, unable to enqueue buffer\n");
        }
    }
    ring_init(sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].queue_h,
            (ring_buffer_t *)serial_tx_free,
            (ring_buffer_t *)serial_tx_used,
            true,
            NUM_BUFFERS,
            NUM_BUFFERS);
    for (int i = 0; i < NUM_BUFFERS - 1; i++) {
        int ret = enqueue_free(sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].queue_h, serial_tx_data + (i * BUFFER_SIZE), BUFFER_SIZE, NULL);
        assert(ret == 0);
        if (ret != 0) {
            microkit_dbg_puts(microkit_name);
            microkit_dbg_puts(": server tx buffer population, unable to enqueue buffer\n");
        }
    }

    /* Initialise virtIO console device */
    success = virtio_mmio_device_init(&virtio_console, CONSOLE, VIRTIO_CONSOLE_BASE, VIRTIO_CONSOLE_SIZE, VIRTIO_CONSOLE_IRQ, sddf_serial_handlers);
    assert(success);

    /* virtIO block */
    sddf_blk_handlers[SDDF_BLK_DEFAULT_HANDLE].queue_h = &blk_queue_h;
    sddf_blk_handlers[SDDF_BLK_DEFAULT_HANDLE].config = (void *)blk_config;
    sddf_blk_handlers[SDDF_BLK_DEFAULT_HANDLE].data = (uintptr_t)blk_data;
    sddf_blk_handlers[SDDF_BLK_DEFAULT_HANDLE].ch = BLK_CH;
    
    /* Initialise our sDDF ring buffers for the block device */
    blk_queue_init(sddf_blk_handlers[SDDF_BLK_DEFAULT_HANDLE].queue_h,
                (blk_req_queue_t *)blk_req_queue,
                (blk_resp_queue_t *)blk_resp_queue,
                true,
                BLK_REQ_QUEUE_SIZE,
                BLK_RESP_QUEUE_SIZE);

    /* Initialise virtIO block device */
    success = virtio_mmio_device_init(&virtio_blk, BLOCK, VIRTIO_BLK_BASE, VIRTIO_BLK_SIZE, VIRTIO_BLK_IRQ, sddf_blk_handlers);
    assert(success);

    /* Finally start the guest */
    guest_start(GUEST_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
}

void notified(microkit_channel ch) {
    switch (ch) {
        case SERIAL_MUX_RX_CH: {
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
void fault(microkit_id id, microkit_msginfo msginfo) {
    bool success = fault_handle(id, msginfo);
    if (success) {
        /* Now that we have handled the fault successfully, we reply to it so
         * that the guest can resume execution. */
        microkit_fault_reply(microkit_msginfo_new(0, 0));
    }
}
