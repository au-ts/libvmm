#include "sddf_snd_shared_ringbuffer.h"
#include "stream.h"
#include "uio.h"
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#define PAGE_SIZE_4K 0x1000
#define RING_BYTES 0x200000

#define SOUND_DEVICE "default"
#define MAX_STREAMS 2
#define UIO_POLLFD 0

#define UIO_MAP "/sys/class/uio/uio0/maps/map"
#define UIO_ADDR(m) UIO_MAP m "/addr"
#define UIO_SIZE(m) UIO_MAP m "/size"
#define UIO_OFFSET(m) UIO_MAP m "/offset"

#define SHARED_STATE_SLOT 0
#define SHARED_STATE_SIZE UIO_SIZE("0")
#define RINGS_SLOT 1
#define RINGS_SIZE UIO_SIZE("1")
#define TX_DATA_SLOT 2
#define TX_DATA_ADDR UIO_ADDR("2")
#define TX_DATA_SIZE UIO_SIZE("2")
#define RX_DATA_SLOT 3
#define RX_DATA_ADDR UIO_ADDR("3")
#define RX_DATA_SIZE UIO_SIZE("3")

typedef struct translation_state {
    ssize_t tx_offset;
    ssize_t rx_offset;
} translation_state_t;

typedef struct driver_state {
    stream_t *streams[MAX_STREAMS];
    int stream_count;

    sddf_snd_shared_state_t *shared_state;
    sddf_snd_rings_t rings;
    translation_state_t translate;
} driver_state_t;

static void signal_ready_to_vmm()
{
    printf("UIO SND|INFO: faulting to VMM\n");
    char command_str[64] = { 0 };
    sprintf(command_str, "devmem %d", UIO_SND_FAULT_ADDRESS);
    int res = system(command_str);
    assert(res == 0);
}

static void print_bytes(void *data)
{
    for (int i = 0; i < 16; i++) {
        printf("%02x ", ((uint8_t *)data)[i]);
    }
    printf("\n");
}

static int get_uio_map_value(const char *path, size_t *value)
{
    // Get size of map0 from UIO device
    size_t fp = open(path, O_RDONLY);
    if (fp == -1) {
        perror("Error opening file");
        return -1;
    }

    char str[64]; // Buffer to hold the string representation of the size
    int str_values_read = read(fp, str, sizeof(str));

    if (str_values_read <= 0) {
        perror("Error reading from file");
        close(fp);
        return -1;
    }

    if (str_values_read == sizeof(str)) {
        fprintf(stderr, "Size string is too long to fit in size_str buffer\n");
        close(fp);
        return -1;
    }

    str[str_values_read] = '\0';
    sscanf(str, "%lx", value);

    close(fp);
    return 0;
}

static void *map_uio(int index, const char *size_str, int uio_fd)
{
    size_t size;
    int res;

    res = get_uio_map_value(size_str, &size);
    if (res) {
        printf("failed to get value\n");
        return MAP_FAILED;
    }

    // "To map the memory of mapping N, you have to use N times the page size as your offset"
    // https://www.kernel.org/doc/html/v4.13/driver-api/uio-howto.html
    off_t offset = index * PAGE_SIZE_4K;

    return mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, uio_fd, offset);
}

int init_mappings(driver_state_t *state, int uio_fd)
{
    int err;

    state->shared_state = map_uio(SHARED_STATE_SLOT, SHARED_STATE_SIZE, uio_fd);
    if (state->shared_state == MAP_FAILED) {
        perror("Error mapping shared_state");
        return -1;
    }

    void *ring_buffers = map_uio(RINGS_SLOT, RINGS_SIZE, uio_fd);
    if (ring_buffers == MAP_FAILED) {
        perror("Error mapping ring_buffers");
        return -1;
    }

    // TODO: tx and rx data are now in wrong position
    // need to add back get addr functions
    // then either fix address or use offset
    void *tx_data = map_uio(TX_DATA_SLOT, TX_DATA_SIZE, uio_fd);
    if (tx_data == MAP_FAILED) {
        perror("Error mapping tx_data");
        return -1;
    }

    void *rx_data = map_uio(RX_DATA_SLOT, RX_DATA_SIZE, uio_fd);
    if (rx_data == MAP_FAILED) {
        perror("Error mapping rx_data");
        return -1;
    }

    size_t tx_data_physical, rx_data_physical;

    err = get_uio_map_value(TX_DATA_ADDR, &tx_data_physical);
    if (err) {
        perror("Error getting tx_data_physical");
        return -1;
    }

    err = get_uio_map_value(RX_DATA_ADDR, &rx_data_physical);
    if (err) {
        perror("Error getting rx_data_physical");
        return -1;
    }

    int offset = 0;
    state->rings.rx_free = (void *)(ring_buffers + RING_BYTES * offset++);
    state->rings.rx_used = (void *)(ring_buffers + RING_BYTES * offset++);
    state->rings.tx_free = (void *)(ring_buffers + RING_BYTES * offset++);
    state->rings.tx_used = (void *)(ring_buffers + RING_BYTES * offset++);
    state->rings.commands = (void *)(ring_buffers + RING_BYTES * offset++);
    state->rings.cmd_responses = (void *)(ring_buffers + RING_BYTES * offset++);
    state->rings.tx_responses = (void *)(ring_buffers + RING_BYTES * offset++);

    state->translate.tx_offset = tx_data - (void *)tx_data_physical;
    state->translate.rx_offset = rx_data - (void *)rx_data_physical;

    return 0;
}

static void handle_cmd(driver_state_t *state, sddf_snd_command_t *cmd)
{
    printf("UIO SND|INFO: got command %s\n", sddf_snd_command_code_str(cmd->code));

    sddf_snd_status_code_t status = SDDF_SND_S_OK;

    // Check stream ID. Currently switch is redundant but we might want to add
    // jack commands in future.
    switch (cmd->code) {
    case SDDF_SND_CMD_PCM_SET_PARAMS:
    case SDDF_SND_CMD_PCM_PREPARE:
    case SDDF_SND_CMD_PCM_RELEASE:
    case SDDF_SND_CMD_PCM_START:
    case SDDF_SND_CMD_PCM_STOP:
        if (cmd->stream_id >= state->stream_count) {
            printf("UIO SND|ERR: stream %d does not exist\n", cmd->stream_id);
            status = SDDF_SND_S_BAD_MSG;
            break;
        }
    }

    switch (cmd->code) {
    case SDDF_SND_CMD_PCM_SET_PARAMS: {
        status = stream_set_params(state->streams[cmd->stream_id], &cmd->set_params);
        break;
    }
    case SDDF_SND_CMD_PCM_PREPARE:
        status = stream_prepare(state->streams[cmd->stream_id]);
        break;
    case SDDF_SND_CMD_PCM_RELEASE:
        status = stream_release(state->streams[cmd->stream_id]);
        break;
    case SDDF_SND_CMD_PCM_START:
        status = stream_start(state->streams[cmd->stream_id]);
        break;
    case SDDF_SND_CMD_PCM_STOP:
        status = stream_stop(state->streams[cmd->stream_id]);
        break;
    default:
        printf("UIO SND|ERR: unknown command %d\n", cmd->code);
        status = SDDF_SND_S_BAD_MSG;
    }

    sddf_snd_enqueue_response(state->rings.cmd_responses, cmd->cookie, status);
}

static void *translate_tx(translation_state_t *translate, void *phys_addr)
{
    return phys_addr + translate->tx_offset;
}

static void *translate_rx(translation_state_t *translate, void *phys_addr)
{
    return phys_addr + translate->rx_offset;
}

static void update_playback(driver_state_t *state, stream_t *playback_stream, bool *signal_vmm)
{
    stream_update(playback_stream);

    if (!stream_accepting_pcm(playback_stream)) {
        return;
    }

    stream_flush_status_t status;

    if (!stream_has_pcm(playback_stream)) {
        sddf_snd_pcm_data_t pcm;
        if (sddf_snd_dequeue_pcm_data(state->rings.tx_used, &pcm) != 0) {
            return;
        }
        void *data = translate_tx(&state->translate, (void *)pcm.addr);
        stream_set_pcm(playback_stream, &pcm, data);
    }

    while ((status = stream_flush_pcm(playback_stream)) == STREAM_FLUSH_NEED_MORE) {

        sddf_snd_pcm_data_t *stream_pcm = stream_get_pcm(playback_stream);

        stream_pcm->len = SDDF_SND_PCM_BUFFER_SIZE;
        sddf_snd_enqueue_pcm_data(state->rings.tx_free, stream_pcm);
        *signal_vmm = true;

        sddf_snd_enqueue_response(state->rings.tx_responses, stream_pcm->cookie, SDDF_SND_S_OK);

        sddf_snd_pcm_data_t pcm;
        if (sddf_snd_dequeue_pcm_data(state->rings.tx_used, &pcm) != 0) {
            return;
        }
        stream_set_pcm(playback_stream, &pcm, translate_tx(&state->translate, (void *)pcm.addr));
    }

    // todo: think about how to handle errors
    if (status == STREAM_FLUSH_ERR) {
        printf("UIO SND|ERR: Failed to flush pcm\n");

        sddf_snd_pcm_data_t *stream_pcm = stream_get_pcm(playback_stream);

        stream_pcm->len = SDDF_SND_PCM_BUFFER_SIZE;
        sddf_snd_enqueue_pcm_data(state->rings.tx_free, stream_pcm);
        *signal_vmm = true;

        sddf_snd_enqueue_response(state->rings.tx_responses, stream_pcm->cookie, SDDF_SND_S_OK);
    }
}

static void handle_interrupt(driver_state_t *state, bool *signal_vmm)
{
    printf("UIO SND|INFO: got interrupt\n");

    sddf_snd_command_t cmd;
    while (sddf_snd_dequeue_cmd(state->rings.commands, &cmd) == 0) {
        handle_cmd(state, &cmd);
        *signal_vmm = true;
    }

    // TODO: enforce seq ordering
    for (int i = 0; i < state->stream_count; i++) {
        stream_t *stream = state->streams[i];
        if (stream_direction(stream) == SND_PCM_STREAM_PLAYBACK) {
            update_playback(state, state->streams[i], signal_vmm);
            break;
        }
    }
}

int update_pollfds(stream_t **streams, int stream_count, struct pollfd *pollfds, int *fd_to_stream,
                   int *stream_to_fds)
{
    // Skip UIO fd
    int open_fd_count = 1;

    for (int i = 0; i < stream_count; i++) {
        stream_t *stream = streams[i];
        struct pollfd *streamfds = stream_fds(stream);

        int nfds = stream_nfds(stream);

        if (!stream_should_poll(stream)) {
            stream_to_fds[i] = -1;
            continue;
        }

        stream_to_fds[i] = open_fd_count;
        for (int j = 0; j < nfds; j++, open_fd_count++) {
            pollfds[open_fd_count] = streamfds[j];
            fd_to_stream[open_fd_count] = i;
        }
    }
    return open_fd_count;
}

int main(void)
{
    int uio_fd = open("/dev/uio0", O_RDWR);
    if (uio_fd == -1) {
        perror("Error opening /dev/uio0");
        return EXIT_FAILURE;
    }
    printf("UIO SND|INFO: opened /dev/uio0\n");

    driver_state_t state;
    int err = init_mappings(&state, uio_fd);
    if (err) {
        printf("UIO SND|ERR: failed to initialise mappings\n");
        return EXIT_FAILURE;
    }

    // The idea is that this should work even if one stream fails to open.
    snd_pcm_stream_t target_streams[MAX_STREAMS] = {
        SND_PCM_STREAM_PLAYBACK,
        SND_PCM_STREAM_CAPTURE,
    };
    sddf_snd_ring_state_t *stream_consume_rings[MAX_STREAMS] = {
        &state.rings.tx_used->state,
        &state.rings.rx_free->state,
    };

    state.stream_count = 0;

    for (int i = 0; i < MAX_STREAMS; i++) {
        state.streams[state.stream_count]
            = stream_open(&state.shared_state->stream_info[state.stream_count], SOUND_DEVICE,
                          target_streams[i], stream_consume_rings[i]);

        if (state.streams[state.stream_count] == NULL)
            printf("Failed to initialise target stream %d\n", i);
        else
            state.stream_count++;
    }

    state.shared_state->streams = state.stream_count;
    printf("UIO SND|INFO: %d streams available\n", state.stream_count);

    // Signal ready even if we don't create all streams.
    signal_ready_to_vmm();

    if (state.stream_count == 0) {
        printf("No streams available, exiting\n");
        return EXIT_FAILURE;
    }

    // Start with 1 for UIO fd
    int fd_count = 1;
    for (int i = 0; i < state.stream_count; i++) {
        fd_count += stream_nfds(state.streams[i]);
    }

    struct pollfd *fds = calloc(fd_count, sizeof(struct pollfd));
    int *fd_to_stream = calloc(fd_count, sizeof(int));
    int *stream_to_fds = calloc(state.stream_count, sizeof(int));

    fds[UIO_POLLFD].fd = uio_fd;
    fds[UIO_POLLFD].events = POLLIN;
    fds[UIO_POLLFD].revents = 0;
    fd_to_stream[UIO_POLLFD] = -1;

    int curr_fd_count
        = update_pollfds(state.streams, state.stream_count, fds, fd_to_stream, stream_to_fds);

    // int expire = 10;

    while (true) {
        printf("UIO SND|INFO: polling with %d fds\n", curr_fd_count);

        // We might not need a UIO interrupt for tx/rx, ALSA should signal.
        int ready = poll(fds, curr_fd_count, -1);
        if (ready == -1) {
            perror("UIO SND|ERR: Failed to poll descriptors");
            return EXIT_FAILURE;
        }

        printf("UIO SND|INFO: Awake. Stream state: \n");
        stream_debug_state(state.streams[0]);
        printf("UIO SND|INFO: tx_used size %d\n", sddf_snd_ring_size(&state.rings.tx_used->state));

        bool signal_vmm = false;

        if (fds[UIO_POLLFD].revents & POLLIN) {
            handle_interrupt(&state, &signal_vmm);
        }

        for (int i = 0; i < state.stream_count; i++) {
            if (stream_to_fds[i] == -1) {
                continue;
            }

            stream_t *stream = state.streams[i];
            int fd_begin = stream_to_fds[i];
            int fd_end = fd_begin + stream_nfds(stream);

            unsigned short revents;
            stream_demangle_fds(stream, &fds[fd_begin], stream_nfds(stream), &revents);

            for (int fd_index = fd_begin; fd_index < fd_end; fd_index++) {
                if (fds[fd_index].revents & POLLIN) {
                    printf("UIO SND|INFO: got poll event on stream %d (idx %d)\n", i, fd_index);

                    if (stream_direction(stream) == SND_PCM_STREAM_PLAYBACK) {
                        update_playback(&state, stream, &signal_vmm);
                    }
                    break;
                }
            }
        }

        // Playback: only include the file descriptor if we have written a pcm frame
        //  and tx used is non-empty
        // Recording: only include file descriptor if rx free is non-empty

        if (signal_vmm) {
            signal_ready_to_vmm();
        }

        // if (expire-- < 0)
        //     break;

        printf("Updating pollfds\n");
        curr_fd_count
            = update_pollfds(state.streams, state.stream_count, fds, fd_to_stream, stream_to_fds);
    }

    for (int i = 0; i < state.stream_count; i++) {
        stream_close(state.streams[i]);
    }

    free(fds);
    free(fd_to_stream);
    free(stream_to_fds);
    return EXIT_SUCCESS;
}
