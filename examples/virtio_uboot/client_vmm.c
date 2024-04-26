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
#include <uboot.h>
#include <fault.h>
#include <guest.h>
#include <virq.h>
#include <tcb.h>
#include <vcpu.h>
#include <virtio/virtio.h>
#include <virtio/console.h>
#include <virtio/block.h>
#include <sddf/serial/queue.h>
#include <sddf/blk/queue.h>

#define GUEST_RAM_SIZE 0x6000000

/******
 * 
 * 
 * 
 * THIS FILE USES THE UBOOT VARIANTS OF THE INSTRUCTIONS TO LOAD INSTEAD OF
 * THE LINUX VARIANTS. NOTE THAT WE CURRENTLY USE THE FILE client_vmm.c AND THIS IS
 * JUST KEPT AS AN EXAMPLE.
 * 
 * 
 * 
*/

// TODO - PROBABLY NEED TO CHANGE THESE ADDRESSES
#if defined(BOARD_qemu_arm_virt)
#define GUEST_DTB_VADDR 0x47f00000
#define UBOOT_OFFSET 0x200000
#elif defined(BOARD_odroidc4)
#define GUEST_DTB_VADDR 0x25f10000
#define UBOOT_OFFSET 0x200000
#else
#error Need to define DTB address
#endif

// TODO - WOULD LIKE TO GENERALISE THIS FILE SO NOT SPECIFIC TO LOADING LINUX KERNEL
/* Data for the guest's kernel image. */
extern char _guest_kernel_image[];
extern char _guest_kernel_image_end[];
/* Data for the device tree to be passed to the kernel. */
extern char _guest_dtb_image[];
extern char _guest_dtb_image_end[];
/* Data for the initial RAM disk to be passed to the kernel. NOTE: THIS IS UNUSED FOR UBOOT */
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

// @ericc: This needs to be less than or equal to memory size / blocksize == 4096
// TODO: auto generate from microkit system file
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

void init(void) {
    // Busy wait until blk device is ready
    // Need to put an empty assembly line to prevent compiler from optimising out the busy wait
    // @ericc: Figure out a better way to do this
    LOG_VMM("begin busy wait\n");
    while (!((blk_storage_info_t *)blk_config)->ready) asm("");
    LOG_VMM("done busy waiting\n");

    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", microkit_name);
    /* Place all the binaries in the right locations before starting the guest */
    size_t uboot_size = _guest_kernel_image_end - _guest_kernel_image; // TODO - UPDATE NAMING FOR THESE
    size_t dtb_size = _guest_dtb_image_end - _guest_dtb_image;

    uintptr_t uboot_pc = uboot_setup_image(guest_ram_vaddr,
                                            (uintptr_t) _guest_kernel_image,
                                            uboot_size,
                                            UBOOT_OFFSET,
                                            (uintptr_t) _guest_dtb_image,
                                            GUEST_DTB_VADDR,
                                            dtb_size
                                            );
    if (!uboot_pc) {
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
    sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].data_size = 0; // @ericc: unused
    sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].ch = SERIAL_MUX_RX_CH;
    
    sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].queue_h = &serial_tx_h;
    sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].config = NULL;
    sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].data = (uintptr_t)serial_tx_data;
    sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].data_size = 0; // @ericc: unused
    sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].ch = SERIAL_MUX_TX_CH;

    /* Initialise our sDDF queues for the serial device */
    serial_queue_init(sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].queue_h,
            (serial_queue_t *)serial_rx_free,
            (serial_queue_t *)serial_rx_active,
            true,
            NUM_ENTRIES,
            NUM_ENTRIES);
    for (int i = 0; i < NUM_ENTRIES - 1; i++) {
        int ret = serial_enqueue_free(sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].queue_h, serial_rx_data + (i * BUFFER_SIZE), BUFFER_SIZE);
        if (ret != 0) {
            microkit_dbg_puts(microkit_name);
            microkit_dbg_puts(": server rx buffer population, unable to enqueue\n");
        }
    }
    serial_queue_init(sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].queue_h,
            (serial_queue_t *)serial_tx_free,
            (serial_queue_t *)serial_tx_active,
            true,
            NUM_ENTRIES,
            NUM_ENTRIES);
    for (int i = 0; i < NUM_ENTRIES - 1; i++) {
        int ret = serial_enqueue_free(sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].queue_h, serial_tx_data + (i * BUFFER_SIZE), BUFFER_SIZE);
        assert(ret == 0);
        if (ret != 0) {
            microkit_dbg_puts(microkit_name);
            microkit_dbg_puts(": server tx buffer population, unable to enqueue\n");
        }
    }

    /* Initialise virtIO console device */
    success = virtio_mmio_device_init(&virtio_console, CONSOLE, VIRTIO_CONSOLE_BASE, VIRTIO_CONSOLE_SIZE, VIRTIO_CONSOLE_IRQ, sddf_serial_handlers);
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
    success = virtio_mmio_device_init(&virtio_blk, BLOCK, VIRTIO_BLK_BASE, VIRTIO_BLK_SIZE, VIRTIO_BLK_IRQ, sddf_blk_handlers);
    assert(success);

    /* Finally start the guest */
    uboot_start(GUEST_VCPU_ID, uboot_pc, GUEST_DTB_VADDR);
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
