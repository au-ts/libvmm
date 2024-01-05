#include <stdbool.h>
#include <stdint.h>
#include <microkit.h>
#include "sound/libsoundsharedringbuffer/include/sddf_snd_shared_ringbuffer.h"
#include "util.h"
#include "fault.h"
#include "uio.h"

#define LOG_DRIVER(...) do{ microkit_dbg_puts(microkit_name); microkit_dbg_puts("|INFO: "); microkit_dbg_puts(__VA_ARGS__); }while(0)
#define LOG_DRIVER_ERR(...) do{ microkit_dbg_puts(microkit_name); microkit_dbg_puts("|ERROR: "); microkit_dbg_puts(__VA_ARGS__); }while(0)

/* Defines to manage interrupts and notifications */
#define DRIVER_VMM_CH 1
#define CLIENT_CH 2

#define RING_SIZE 0x200000

uintptr_t driver_commands;
uintptr_t driver_responses;
uintptr_t driver_rx_free;
uintptr_t driver_rx_used;
uintptr_t driver_tx_free;
uintptr_t driver_tx_used;

uintptr_t uio_commands;
uintptr_t uio_responses;
uintptr_t uio_rx_free;
uintptr_t uio_rx_used;
uintptr_t uio_tx_free;
uintptr_t uio_tx_used;

uintptr_t shared_state;
uintptr_t rx_data;
uintptr_t tx_data;

static sddf_snd_rings_t device_rings;
static sddf_snd_rings_t uio_rings;

void init(void) {
    LOG_DRIVER("initialising\n");

    assert(driver_commands);
    assert(driver_responses);
    assert(driver_rx_free);
    assert(driver_rx_used);
    assert(driver_tx_free);
    assert(driver_tx_used);

    assert(uio_commands);
    assert(uio_responses);
    assert(uio_rx_free);
    assert(uio_rx_used);
    assert(uio_tx_free);
    assert(uio_tx_used);

    // Init the shared ring buffers
    device_rings = (sddf_snd_rings_t){
        .commands = (sddf_snd_cmd_ring_t *)driver_commands,
        .responses = (sddf_snd_response_ring_t *)driver_responses,
        .rx_free  = (sddf_snd_pcm_data_ring_t *)driver_rx_free,
        .rx_used  = (sddf_snd_pcm_data_ring_t *)driver_rx_used,
        .tx_free  = (sddf_snd_pcm_data_ring_t *)driver_tx_free,
        .tx_used  = (sddf_snd_pcm_data_ring_t *)driver_tx_used,
    };
    sddf_snd_rings_init_default(&device_rings);

    LOG_DRIVER("initialised device rings\n");

    uio_rings = (sddf_snd_rings_t){
        .commands = (sddf_snd_cmd_ring_t *)uio_commands,
        .responses = (sddf_snd_response_ring_t *)uio_responses,
        .rx_free  = (sddf_snd_pcm_data_ring_t *)uio_rx_free,
        .rx_used  = (sddf_snd_pcm_data_ring_t *)uio_rx_used,
        .tx_free  = (sddf_snd_pcm_data_ring_t *)uio_tx_free,
        .tx_used  = (sddf_snd_pcm_data_ring_t *)uio_tx_used,
    };
    sddf_snd_rings_init_default(&uio_rings);
    LOG_DRIVER("initialised uio rings\n");

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
    LOG_DRIVER("filled data");

    // Color data
    // memset((void *)uio_shared_state, 0x32, 0x1000);
    // memset((void *)uio_commands, 0x45, 0x200000);
    // memset((void *)uio_responses, 0x53, 0x200000);
    // memset((void *)uio_rx_free, 0x67, 0x200000);
    // memset((void *)uio_rx_used, 0x68, 0x200000);
    // memset((void *)uio_tx_free, 0x9a, 0x200000);
    // memset((void *)uio_tx_used, 0x9b, 0x200000);
    // memset((void *)tx_data, 0xab, 0x200000);
    // memset((void *)rx_data, 0xcd, 0x200000);

    printf("SND command ring size %d\n", sddf_snd_ring_size(&uio_rings.commands->state));
    printf("SND response ring size %d\n", sddf_snd_ring_size(&uio_rings.responses->state));
    printf("SND rx_free ring size %d\n", sddf_snd_ring_size(&uio_rings.rx_free->state));
    printf("SND rx_used ring size %d\n", sddf_snd_ring_size(&uio_rings.rx_used->state));
    printf("SND tx_free ring size %d\n", sddf_snd_ring_size(&uio_rings.tx_free->state));
    printf("SND tx_used ring size %d\n", sddf_snd_ring_size(&uio_rings.tx_used->state));

    LOG_DRIVER("WRITTEN BYTES");

    // todo: initialise shared data
}

void handle_vmm() {
    LOG_DRIVER("Got notification from vmm, notifying client\n");

    sddf_snd_response_t resp;
    // Send responses to client
    while (sddf_snd_dequeue_response(uio_rings.responses, &resp) == 0) {
        LOG_DRIVER("deq uio.responses, enq device.responses\n");
        sddf_snd_enqueue_response(device_rings.responses, resp.cmd_id, resp.status);
    }

    sddf_snd_pcm_data_t pcm;
    // Send recorded PCM frames to client
    while (sddf_snd_dequeue_pcm_data(uio_rings.rx_used, &pcm) == 0) {
        LOG_DRIVER("deq uio.rx_used, enq device.rx_used\n");
        sddf_snd_enqueue_pcm_data(device_rings.rx_used, pcm.stream_id, pcm.addr, pcm.len);
    }
    // Return free playback frames
    while (sddf_snd_dequeue_pcm_data(uio_rings.tx_free, &pcm) == 0) {
        LOG_DRIVER("deq uio.tx_free, enq device.tx_free\n");
        sddf_snd_enqueue_pcm_data(device_rings.tx_free, pcm.stream_id, pcm.addr, pcm.len);
    }

    microkit_notify(CLIENT_CH);
}

void handle_client() {
    LOG_DRIVER("Got notificatioon from client");

    // Forward commands
    sddf_snd_command_t cmd;
    while (sddf_snd_dequeue_cmd(device_rings.commands, &cmd) == 0) {
        LOG_DRIVER("deq device.commands, enq uio.commands\n");
        sddf_snd_enqueue_cmd(uio_rings.commands, &cmd);
    }

    sddf_snd_pcm_data_t pcm;
    // Forward playback frames
    while (sddf_snd_dequeue_pcm_data(device_rings.tx_used, &pcm) == 0) {
        LOG_DRIVER("deq device.tx_used, enq uio.tx_used\n");
        sddf_snd_enqueue_pcm_data(uio_rings.tx_used, pcm.stream_id, pcm.addr, pcm.len);
    }
    // Return free recording frames
    while (sddf_snd_dequeue_pcm_data(device_rings.rx_free, &pcm) == 0) {
        LOG_DRIVER("deq device.rx_free, enq uio.rx_free\n");
        sddf_snd_enqueue_pcm_data(uio_rings.rx_free, pcm.stream_id, pcm.addr, pcm.len);
    }

    microkit_notify(DRIVER_VMM_CH);
}

void notified(microkit_channel ch) {
    switch(ch) {
    case DRIVER_VMM_CH:
        /** The userlevel driver has recv PCM data for us? */
        handle_vmm();
        return;
    case CLIENT_CH:
        /** Someone enqueued commands for us */
        handle_client();
        break;
    default:
        LOG_DRIVER("received notification on unexpected channel\n");
        break;
    }
}
