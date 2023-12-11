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
#include "virtio/sound.h"
#include <sddf/serial/queue.h>
#include <sddf/sound/queue.h>

#if defined(BOARD_qemu_arm_virt)
#define GUEST_DTB_VADDR 0x47000000
#define GUEST_INIT_RAM_DISK_VADDR 0x46000000
#elif defined(BOARD_odroidc4)
#define GUEST_DTB_VADDR 0x27000000
#define GUEST_INIT_RAM_DISK_VADDR 0x26000000
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

#define VIRTIO_SOUND_IRQ (76)
#define VIRTIO_SOUND_BASE (0x170000)
#define VIRTIO_SOUND_SIZE (0x1000)

#define SOUND_DRIVER_CH 4

uintptr_t serial_rx_free;
uintptr_t serial_rx_active;
uintptr_t serial_tx_free;
uintptr_t serial_tx_active;

uintptr_t serial_rx_data;
uintptr_t serial_tx_data;

static serial_queue_handle_t serial_rx_h;
static serial_queue_handle_t serial_tx_h;
static sddf_handler_t sddf_serial_handlers[SDDF_SERIAL_NUM_HANDLES];

uintptr_t sound_cmd_req;
uintptr_t sound_cmd_res;
uintptr_t sound_pcm_req;
uintptr_t sound_pcm_res;

uintptr_t sound_data;

uintptr_t sound_shared_state;

static sddf_handler_t sound_handler;
static sound_state_t sound_state;

static struct virtio_device virtio_console;
static struct virtio_device virtio_sound;

static bool guest_started = false;
uintptr_t kernel_pc = 0;

void init(void) {
    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", microkit_name);
    /* Place all the binaries in the right locations before starting the guest */
    size_t kernel_size = _guest_kernel_image_end - _guest_kernel_image;
    size_t dtb_size = _guest_dtb_image_end - _guest_dtb_image;
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;
    kernel_pc = linux_setup_images(guest_ram_vaddr,
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
    serial_queue_init(sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].queue_h,
        (serial_queue_t *)serial_rx_free,
        (serial_queue_t *)serial_rx_active,
        true,
        NUM_ENTRIES,
        NUM_ENTRIES);
    for (int i = 0; i < NUM_ENTRIES - 1; i++) {
        int ret = serial_enqueue_free(sddf_serial_handlers[SDDF_SERIAL_RX_HANDLE].queue_h,
                               serial_rx_data + (i * BUFFER_SIZE),
                               BUFFER_SIZE);
        if (ret != 0) {
            microkit_dbg_puts(microkit_name);
            microkit_dbg_puts(": server rx buffer population, unable to enqueue buffer\n");
        }
    }
    serial_queue_init(sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].queue_h,
            (serial_queue_t *)serial_tx_free,
            (serial_queue_t *)serial_tx_active,
            true,
            NUM_ENTRIES,
            NUM_ENTRIES);
    for (int i = 0; i < NUM_ENTRIES - 1; i++) {
        // Have to start at the memory region left of by the rx ring
        int ret = serial_enqueue_free(sddf_serial_handlers[SDDF_SERIAL_TX_HANDLE].queue_h,
                               serial_tx_data + ((i + NUM_ENTRIES) * BUFFER_SIZE),
                               BUFFER_SIZE);
        assert(ret == 0);
        if (ret != 0) {
            microkit_dbg_puts(microkit_name);
            microkit_dbg_puts(": server tx buffer population, unable to enqueue buffer\n");
        }
    }

    /* Initialise virtIO console device */
    success = virtio_mmio_device_init(&virtio_console, CONSOLE, VIRTIO_CONSOLE_BASE,
                                      VIRTIO_CONSOLE_SIZE, VIRTIO_CONSOLE_IRQ, sddf_serial_handlers);
    assert(success);

    assert(sound_cmd_req);
    assert(sound_cmd_res);
    assert(sound_pcm_req);
    assert(sound_pcm_res);
    assert(sound_data);

    sound_state.shared_state = (sound_shared_state_t *)sound_shared_state;
    sound_state.queues = (sound_queues_t){
        .cmd_req = (void *)sound_cmd_req,
        .cmd_res = (void *)sound_cmd_res,
        .pcm_req = (void *)sound_pcm_req,
        .pcm_res = (void *)sound_pcm_res,
    };
    sound_queues_init_default(&sound_state.queues);

    // @alexbr: why -1?
    for (int i = 0; i < SOUND_NUM_BUFFERS - 1; i++) {
        sound_pcm_t pcm;
        memset(&pcm, 0, sizeof(pcm));
        pcm.len = SOUND_PCM_BUFFER_SIZE;

        pcm.addr = sound_data + (i * SOUND_PCM_BUFFER_SIZE);
        int ret = sound_enqueue_pcm(sound_state.queues.pcm_res, &pcm);
        assert(ret == 0);
    }

    sound_handler.queue_h = &sound_state;
    sound_handler.config = NULL;
    sound_handler.data = 0;
    sound_handler.ch = SOUND_DRIVER_CH;

    success = virtio_mmio_device_init(&virtio_sound, SOUND,
                                      VIRTIO_SOUND_BASE, VIRTIO_SOUND_SIZE,
                                      VIRTIO_SOUND_IRQ, &sound_handler);
    assert(success);
    
    /* Don't start the guest until driver VM is ready. */
}

void notified(microkit_channel ch) {
    switch (ch) {
        case SERIAL_MUX_RX_CH: {
            /* We have received an event from the serial multipelxor, so we
             * call the virtIO console handling */
            virtio_console_handle_rx(&virtio_console);
            break;
        }
        case SOUND_DRIVER_CH: {
            virtio_snd_notified(&virtio_sound);
            if (!guest_started) {
                guest_start(GUEST_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
                guest_started = true;
            }
            break;
        }
        default:
            printf("Unexpected channel, ch: 0x%lx\n", ch);
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
