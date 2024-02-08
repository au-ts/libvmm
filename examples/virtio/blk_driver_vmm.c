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

/*
 * As this is just an example, for simplicity we just make the size of the
 * guest's "RAM" the same for all platforms. For just booting Linux with a
 * simple user-space, 0x10000000 bytes (256MB) is plenty.
 */
#define GUEST_RAM_SIZE 0x8000000

#if defined(BOARD_qemu_arm_virt)
#define GUEST_DTB_VADDR 0x47f00000
#define GUEST_INIT_RAM_DISK_VADDR 0x47000000
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

#define MAX_IRQ_CH 63
int passthrough_irq_map[MAX_IRQ_CH];

static void dummy_ack(size_t vcpu_id, int irq, void *cookie) {
    return;
}

static void passthrough_device_ack(size_t vcpu_id, int irq, void *cookie) {
    microkit_channel irq_ch = (microkit_channel)(int64_t)cookie;
    microkit_irq_ack(irq_ch);
}

static void register_passthrough_irq(int irq, microkit_channel irq_ch) {
    LOG_VMM("Register passthrough IRQ %d (channel: 0x%lx)\n", irq, irq_ch);
    assert(irq_ch < MAX_IRQ_CH);
    passthrough_irq_map[irq_ch] = irq;

    int err = virq_register(GUEST_VCPU_ID, irq, &passthrough_device_ack, (void *)(int64_t)irq_ch);
    if (!err) {
        LOG_VMM_ERR("Failed to register IRQ %d\n", irq);
        return;
    }
}

/* sDDF block */
typedef struct {
    int irq;
    int ch;
} uio_device_t;

#define MAX_UIO_DEVICE 32
// @ericc: @TODO: autogen these, irq from dts, ch from microkit system file
#define NUM_UIO_DEVICE 2
uio_device_t uio_devices[MAX_UIO_DEVICE] = {
    { .irq = 50, .ch = 3 },
    { .irq = 51, .ch = 4 },
};

// @ericc: @TODO: change from linear search to O(1) later
static int get_uio_ch(int irq) {
    for (int i = 0; i < MAX_UIO_DEVICE; i++) {
        if (uio_devices[i].irq == irq) {
            return uio_devices[i].ch;
        }
    }
    return -1;
}

static int get_uio_irq_from_ch(int ch) {
    for (int i = 0; i < MAX_UIO_DEVICE; i++) {
        if (uio_devices[i].ch == ch) {
            return uio_devices[i].irq;
        }
    }
    return -1;
}

void uio_ack(size_t vcpu_id, int irq, void *cookie) {
    microkit_notify(get_uio_ch(irq));
}

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

size_t serial_ch[SDDF_SERIAL_NUM_CH];

ring_handle_t serial_rx_ring_handle;
ring_handle_t serial_tx_ring_handle;

static ring_handle_t *serial_ring_handles[SDDF_SERIAL_NUM_HANDLES];

static struct virtio_device virtio_console;

void init(void) {
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
    ring_init(&serial_rx_ring_handle,
            (ring_buffer_t *)serial_rx_free,
            (ring_buffer_t *)serial_rx_used,
            true,
            NUM_BUFFERS,
            NUM_BUFFERS);
    for (int i = 0; i < NUM_BUFFERS - 1; i++) {
        int ret = enqueue_free(&serial_rx_ring_handle, serial_rx_data + (i * BUFFER_SIZE), BUFFER_SIZE, NULL);
        if (ret != 0) {
            microkit_dbg_puts(microkit_name);
            microkit_dbg_puts(": server rx buffer population, unable to enqueue buffer\n");
        }
    }
    ring_init(&serial_tx_ring_handle,
            (ring_buffer_t *)serial_tx_free,
            (ring_buffer_t *)serial_tx_used,
            true,
            NUM_BUFFERS,
            NUM_BUFFERS);
    for (int i = 0; i < NUM_BUFFERS - 1; i++) {
        // Have to start at the memory region left of by the rx ring
        int ret = enqueue_free(&serial_tx_ring_handle, serial_tx_data + ((i + NUM_BUFFERS) * BUFFER_SIZE), BUFFER_SIZE, NULL);
        assert(ret == 0);
        if (ret != 0) {
            microkit_dbg_puts(microkit_name);
            microkit_dbg_puts(": server tx buffer population, unable to enqueue buffer\n");
        }
    }
    serial_ring_handles[SDDF_SERIAL_RX_RING] = &serial_rx_ring_handle;
    serial_ring_handles[SDDF_SERIAL_TX_RING] = &serial_tx_ring_handle;
    /* Neither ring should be plugged and hence all buffers we send should actually end up at the driver. */
    assert(!ring_plugged(serial_tx_ring_handle.free_ring));
    assert(!ring_plugged(serial_tx_ring_handle.used_ring));
    /* Initialise channel */
    serial_ch[SDDF_SERIAL_TX_CH_INDEX] = SERIAL_MUX_TX_CH;
    /* Initialise virtIO console device */
    success = virtio_mmio_device_init(&virtio_console, CONSOLE, VIRTIO_CONSOLE_BASE, VIRTIO_CONSOLE_SIZE, VIRTIO_CONSOLE_IRQ,
                                      NULL, NULL, (void **)serial_ring_handles, serial_ch);
    assert(success);

    /* Register UIO irq */
    for (int i = 0; i < NUM_UIO_DEVICE; i++) {
        virq_register(GUEST_VCPU_ID, uio_devices[i].irq, &uio_ack, NULL);
    }

    /* Finally start the guest */
    guest_start(GUEST_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
}

void notified(microkit_channel ch) {
    /* Handle notifications from clients of block device */
    int irq = get_uio_irq_from_ch(ch);
    if (irq != -1) {
        virq_inject(GUEST_VCPU_ID, irq);
        return;
    }

    switch (ch) {
        case SERIAL_MUX_RX_CH: {
            /* We have received an event from the serial multipelxor, so we
             * call the virtIO console handling */
            virtio_console_handle_rx(&virtio_console);
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
