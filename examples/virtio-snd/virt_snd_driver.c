#include <stdbool.h>
#include <stdint.h>
#include <microkit.h>
#include "sound/libsoundsharedringbuffer/include/sddf_snd_shared_ringbuffer.h"
#include "util.h"

#define LOG_DRIVER(...) do{ microkit_dbg_puts(microkit_name); microkit_dbg_puts("|INFO: "); microkit_dbg_puts(__VA_ARGS__); }while(0)
#define LOG_DRIVER_ERR(...) do{ microkit_dbg_puts(microkit_name); microkit_dbg_puts("|ERROR: "); microkit_dbg_puts(__VA_ARGS__); }while(0)

/* Defines to manage interrupts and notifications */
#define VMM_CH 1
#define CMD_CH  2
#define TX_CH  3
#define RX_CH  4

#define RING_SIZE 0x200000

uintptr_t driver_shared_state;
uintptr_t driver_commands;
uintptr_t driver_rx_free;
uintptr_t driver_rx_used;
uintptr_t driver_tx_free;
uintptr_t driver_tx_used;

uintptr_t rx_data;
uintptr_t tx_data;

uintptr_t uio_shared_state;
uintptr_t uio_ring_buffers;

static sddf_snd_rings_t device_rings;
static sddf_snd_rings_t uio_rings;

void init(void) {
    LOG_DRIVER("initialising\n");

    assert(driver_commands);
    assert(driver_rx_free);
    assert(driver_rx_used);
    assert(driver_tx_free);
    assert(driver_tx_used);
    assert(uio_ring_buffers);

    // Init the shared ring buffers
    device_rings = (sddf_snd_rings_t){
        .commands = (sddf_snd_cmd_ring_t *)driver_commands,
        .rx_free  = (sddf_snd_pcm_data_ring_t *)driver_rx_free,
        .rx_used  = (sddf_snd_pcm_data_ring_t *)driver_rx_used,
        .tx_free  = (sddf_snd_pcm_data_ring_t *)driver_tx_free,
        .tx_used  = (sddf_snd_pcm_data_ring_t *)driver_tx_used,
    };
    sddf_snd_rings_init_default(&device_rings);

    uio_rings = (sddf_snd_rings_t){
        .commands = (sddf_snd_cmd_ring_t *)uio_ring_buffers,
        .rx_free  = (sddf_snd_pcm_data_ring_t *)(uio_ring_buffers + RING_SIZE),
        .rx_used  = (sddf_snd_pcm_data_ring_t *)(uio_ring_buffers + RING_SIZE * 2),
        .tx_free  = (sddf_snd_pcm_data_ring_t *)(uio_ring_buffers + RING_SIZE * 3),
        .tx_used  = (sddf_snd_pcm_data_ring_t *)(uio_ring_buffers + RING_SIZE * 4),
    };
    sddf_snd_rings_init_default(&uio_rings);

    // @alexbr: why -1?
    for (int i = 0; i < SDDF_SND_NUM_PCM_DATA_BUFFERS - 1; i++) {
        // UIO gets free RX buffers as it will need them first.
        int ret;
        ret = sddf_snd_enqueue_pcm_data(uio_rings.rx_free, (uint32_t)-1,
                                        rx_data + (i * SDDF_SND_PCM_BUFFER_SIZE),
                                        SDDF_SND_PCM_BUFFER_SIZE);
        assert(ret == 0);
        // Client gets free TX buffers as it will need them first.
        ret = sddf_snd_enqueue_pcm_data(device_rings.tx_free, (uint32_t)-1,
                                        tx_data + (i * SDDF_SND_PCM_BUFFER_SIZE),
                                        SDDF_SND_PCM_BUFFER_SIZE);
        assert(ret == 0);
    }

    // todo: initialise shared data
}

void handle_vmm() {
    LOG_DRIVER("Got vmm notification!");
    // todo: first time this is called, recv data from vmm?
}

void handle_tx() {
    LOG_DRIVER("Got tx notification!");
}

void notified(microkit_channel ch) {
    switch(ch) {
    case VMM_CH:
        /** The userlevel driver has recv PCM data for us? */
        handle_vmm();
        return;
    case CMD_CH:
        /** Someone enqueued commands for us */
        handle_tx();
        break;
    case TX_CH:
        /** Someone enqueued PCM frames for us */
        handle_tx();
        break;
    case RX_CH:
        LOG_DRIVER_ERR("Should not be getting RX notifications");
        break;
    default:
        LOG_DRIVER("received notification on unexpected channel\n");
        break;
    }
}
