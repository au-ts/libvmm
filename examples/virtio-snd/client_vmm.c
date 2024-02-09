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
#include "sddf/serial/shared_ringbuffer.h"
#include "sound/libsoundsharedringbuffer/include/sddf_snd_shared_ringbuffer.h"

/*
 * As this is just an example, for simplicity we just make the size of the
 * guest's "RAM" the same for all platforms. For just booting Linux with a
 * simple user-space, 0x10000000 bytes (256MB) is plenty.
 */
#define GUEST_RAM_SIZE 0x10000000

#if defined(BOARD_qemu_arm_virt)
#define GUEST_DTB_VADDR 0x47000000
#define GUEST_INIT_RAM_DISK_VADDR 0x46000000
#elif defined(BOARD_odroidc4)
#define GUEST_DTB_VADDR 0x2f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x2d700000
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
uintptr_t serial_rx_used;
uintptr_t serial_tx_free;
uintptr_t serial_tx_used;

uintptr_t serial_rx_data;
uintptr_t serial_tx_data;

static ring_handle_t serial_rx_ring_handle;
static ring_handle_t serial_tx_ring_handle;

static size_t serial_ch[SDDF_SERIAL_NUM_CH];
static ring_handle_t *serial_ring_handles[SDDF_SERIAL_NUM_HANDLES];

uintptr_t sound_commands;
uintptr_t sound_responses;
uintptr_t sound_tx_used;
uintptr_t sound_tx_free;
uintptr_t sound_rx_used;
uintptr_t sound_rx_free;

uintptr_t sound_rx_data;
uintptr_t sound_tx_data;

uintptr_t sound_shared_state;

// @alexbr: currently SDDF_SERIAL_NUM_CH is defined in virtio...
// doesn't make sense
static size_t sound_ch[VIRTIO_SND_NUM_CH];
static sddf_snd_state_t snd_state;

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
    /* Initialise virtIO console device */
    serial_ch[SDDF_SERIAL_TX_CH_INDEX] = SERIAL_MUX_TX_CH;
    success = virtio_mmio_device_init(&virtio_console, CONSOLE, VIRTIO_CONSOLE_BASE, VIRTIO_CONSOLE_SIZE, VIRTIO_CONSOLE_IRQ,
                                      NULL, NULL, (void **)serial_ring_handles, serial_ch);
    assert(success);

    assert(sound_commands);
    assert(sound_responses);
    assert(sound_rx_free);
    assert(sound_rx_used);
    assert(sound_tx_free);
    assert(sound_rx_data);
    assert(sound_tx_data);

    snd_state.shared_state = (sddf_snd_shared_state_t *)sound_shared_state;
    snd_state.rings = (sddf_snd_rings_t){
        .commands  = (void *)sound_commands,
        .responses = (void *)sound_responses,
        .tx_req   = (void *)sound_tx_used,
        .tx_res   = (void *)sound_tx_free,
        .rx_res   = (void *)sound_rx_used,
        .rx_req   = (void *)sound_rx_free,
    };
    sddf_snd_rings_init_default(&snd_state.rings);

    // @alexbr: why -1?
    for (int i = 0; i < SDDF_SND_NUM_BUFFERS - 1; i++) {
        sddf_snd_pcm_data_t pcm;
        memset(&pcm, 0, sizeof(pcm));
        pcm.len = SDDF_SND_PCM_BUFFER_SIZE;

        pcm.addr = sound_rx_data + (i * SDDF_SND_PCM_BUFFER_SIZE);
        int ret = sddf_snd_enqueue_pcm_data(snd_state.rings.rx_res, &pcm);
        assert(ret == 0);

        pcm.addr = sound_tx_data + (i * SDDF_SND_PCM_BUFFER_SIZE);
        ret = sddf_snd_enqueue_pcm_data(snd_state.rings.tx_res, &pcm);
        assert(ret == 0);
    }

    sound_ch[VIRTIO_SND_CH_INDEX] = SOUND_DRIVER_CH;
    success = virtio_mmio_device_init(&virtio_sound, SND, VIRTIO_SOUND_BASE, VIRTIO_SOUND_SIZE, VIRTIO_SOUND_IRQ,
                                      NULL, NULL, (void **)&snd_state, sound_ch);
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
