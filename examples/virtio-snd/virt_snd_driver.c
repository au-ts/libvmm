#include <stdbool.h>
#include <stdint.h>
#include <microkit.h>
#include "sound/libsoundsharedringbuffer/include/sddf_snd_shared_ringbuffer.h"
#include "util.h"
#include "fault.h"
#include "uio.h"

#define LOG_DRIVER(...) do{ microkit_dbg_puts(microkit_name); microkit_dbg_puts("|INFO: "); printf(__VA_ARGS__); }while(0)
#define LOG_DRIVER_ERR(...) do{ microkit_dbg_puts(microkit_name); microkit_dbg_puts("|ERROR: "); printf(__VA_ARGS__); }while(0)

/* Defines to manage interrupts and notifications */
#define DRIVER_VMM_CH 1
#define CLIENT_CH 2

#define RING_SIZE 0x200000

uintptr_t driver_commands;
uintptr_t driver_responses;
uintptr_t driver_tx_free;
uintptr_t driver_rx_used;
uintptr_t driver_rx_free;

uintptr_t uio_commands;
uintptr_t uio_responses;
uintptr_t uio_tx_free;
uintptr_t uio_rx_used;
uintptr_t uio_rx_free;

uintptr_t shared_state;
uintptr_t rx_data;
uintptr_t tx_data;

static sddf_snd_rings_t device_rings;
static sddf_snd_rings_t uio_rings;

void init(void) {
    LOG_DRIVER("Initialising\n");

    assert(driver_commands);
    assert(driver_responses);
    assert(driver_rx_free);
    assert(driver_rx_used);
    assert(driver_tx_free);

    assert(uio_commands);
    assert(uio_responses);
    assert(uio_rx_free);
    assert(uio_rx_used);
    assert(uio_tx_free);

    // Init the shared ring buffers
    device_rings = (sddf_snd_rings_t){
        .commands = (sddf_snd_cmd_ring_t *)driver_commands,
        .responses = (sddf_snd_response_ring_t *)driver_responses,
        .tx_free  = (sddf_snd_pcm_data_ring_t *)driver_tx_free,
        .rx_used  = (sddf_snd_pcm_rx_ring_t *)driver_rx_used,
        .rx_free  = (sddf_snd_pcm_data_ring_t *)driver_rx_free,
    };
    sddf_snd_rings_init_default(&device_rings);

    uio_rings = (sddf_snd_rings_t){
        .commands = (sddf_snd_cmd_ring_t *)uio_commands,
        .responses = (sddf_snd_response_ring_t *)uio_responses,
        .tx_free  = (sddf_snd_pcm_data_ring_t *)uio_tx_free,
        .rx_used  = (sddf_snd_pcm_rx_ring_t *)uio_rx_used,
        .rx_free  = (sddf_snd_pcm_data_ring_t *)uio_rx_free,
    };
    sddf_snd_rings_init_default(&uio_rings);

    // @alexbr: why -1?
    for (int i = 0; i < SDDF_SND_NUM_BUFFERS - 1; i++) {
        sddf_snd_pcm_data_t pcm;
        pcm.len = SDDF_SND_PCM_BUFFER_SIZE;

        // UIO gets free RX buffers as it will need them first.
        pcm.addr = rx_data + (i * SDDF_SND_PCM_BUFFER_SIZE);
        int ret = sddf_snd_enqueue_pcm_data(uio_rings.rx_free, pcm.addr, pcm.len);
        assert(ret == 0);

        // Client gets free TX buffers as it will need them first.
        pcm.addr = tx_data + (i * SDDF_SND_PCM_BUFFER_SIZE);
        ret = sddf_snd_enqueue_pcm_data(device_rings.tx_free, pcm.addr, pcm.len);
        assert(ret == 0);
    }

    LOG_DRIVER("Finished initalising\n");
}

void handle_vmm() {
    // LOG_DRIVER("Got notification from vmm, notifying client\n");

    sddf_snd_response_t resp;
    // Send responses to client
    while (sddf_snd_dequeue_response(uio_rings.responses, &resp) == 0) {
        // LOG_DRIVER("deq uio.cmd_responses, enq device.cmd_responses\n");
        sddf_snd_enqueue_response(device_rings.responses, resp.cookie, resp.status);
    }

    sddf_snd_pcm_rx_t pcm_rx;
    // Send recorded PCM frames to client
    while (sddf_snd_dequeue_pcm_rx(uio_rings.rx_used, &pcm_rx) == 0) {
        // LOG_DRIVER("deq uio.rx_used, enq device.rx_used\n");
        sddf_snd_enqueue_pcm_rx(device_rings.rx_used, &pcm_rx);
    }

    sddf_snd_pcm_data_t pcm;
    // Return free playback frames
    while (sddf_snd_dequeue_pcm_data(uio_rings.tx_free, &pcm) == 0) {
        // LOG_DRIVER("deq uio.tx_free, enq device.tx_free\n");
        assert(pcm.len == SDDF_SND_PCM_BUFFER_SIZE);
        sddf_snd_enqueue_pcm_data(device_rings.tx_free, pcm.addr, pcm.len);
    }

    microkit_notify(CLIENT_CH);
}

void handle_client() {
    // Forward commands
    sddf_snd_command_t cmd;
    while (sddf_snd_dequeue_cmd(device_rings.commands, &cmd) == 0) {
        sddf_snd_enqueue_cmd(uio_rings.commands, &cmd);
    }

    sddf_snd_pcm_data_t pcm;
    // Return free recording frames
    while (sddf_snd_dequeue_pcm_data(device_rings.rx_free, &pcm) == 0) {
        // LOG_DRIVER("deq device.rx_free, enq uio.rx_free\n");
        sddf_snd_enqueue_pcm_data(uio_rings.rx_free, pcm.addr, pcm.len);
    }

    microkit_notify(DRIVER_VMM_CH);
}

void notified(microkit_channel ch) {
    switch(ch) {
    case DRIVER_VMM_CH:
        handle_vmm();
        return;
    case CLIENT_CH:
        handle_client();
        break;
    default:
        LOG_DRIVER("received notification on unexpected channel\n");
        break;
    }
}
