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

typedef struct driver_state {
    stream_t *streams[MAX_STREAMS];
    int stream_count;

    sddf_snd_shared_state_t *shared_state;
    sddf_snd_rings_t rings;
} driver_state_t;

static void signal_ready_to_vmm()
{
    printf("UIO SND|INFO: faulting to VMM\n");
    char command_str[64] = { 0 };
    sprintf(command_str, "devmem %d", UIO_SND_FAULT_ADDRESS);
    int res = system(command_str);
    assert(res == 0);
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

int init_mappings(sddf_snd_shared_state_t **shared_state, sddf_snd_rings_t *rings, int uio_fd)
{
    int err;

    *shared_state = map_uio(SHARED_STATE_SLOT, SHARED_STATE_SIZE, uio_fd);
    if (*shared_state == MAP_FAILED) {
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
    rings->rx_free = (void *)(ring_buffers + RING_BYTES * offset++);
    rings->rx_used = (void *)(ring_buffers + RING_BYTES * offset++);
    rings->tx_free = (void *)(ring_buffers + RING_BYTES * offset++);
    rings->tx_used = (void *)(ring_buffers + RING_BYTES * offset++);
    rings->commands = (void *)(ring_buffers + RING_BYTES * offset++);
    rings->responses = (void *)(ring_buffers + RING_BYTES * offset++);

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

    sddf_snd_enqueue_response(state->rings.responses, cmd->msg_id, status);
}

static void handle_interrupt(driver_state_t *state, bool *signal_vmm)
{
    printf("UIO SND|INFO: got interrupt\n");

    sddf_snd_command_t cmd;
    while (sddf_snd_dequeue_cmd(state->rings.commands, &cmd) == 0) {
        handle_cmd(state, &cmd);
        *signal_vmm = true;
    }

    sddf_snd_pcm_data_t pcm;
    while (sddf_snd_dequeue_pcm_data(state->rings.tx_used, &pcm) == 0) {
        printf("UIO SND|INFO: got playback pcm data: stream %d, addr %lx, len %x\n", pcm.stream_id,
               (size_t)pcm.addr, pcm.len);

        sddf_snd_enqueue_pcm_data(state->rings.tx_free, pcm.stream_id, pcm.addr, SDDF_SND_PCM_BUFFER_SIZE);
        *signal_vmm = true;
    }
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
    int err = init_mappings(&state.shared_state, &state.rings, uio_fd);
    if (err) {
        printf("UIO SND|ERR: failed to initialise mappings\n");
        return EXIT_FAILURE;
    }

    // The idea is that this should work even if one stream fails to open.
    snd_pcm_stream_t target_streams[MAX_STREAMS] = {
        SND_PCM_STREAM_PLAYBACK,
        SND_PCM_STREAM_CAPTURE,
    };

    state.stream_count = 0;

    for (int i = 0; i < MAX_STREAMS; i++) {
        state.streams[state.stream_count] = stream_open(
            &state.shared_state->stream_info[state.stream_count], SOUND_DEVICE, target_streams[i]);

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

    fds[UIO_POLLFD].fd = uio_fd;
    fds[UIO_POLLFD].events = POLLIN;
    fds[UIO_POLLFD].revents = 0;
    fd_to_stream[UIO_POLLFD] = -1;

    // Concatenate stream fd arrays
    int open_fd_count = 1;
    for (int i = 0; i < state.stream_count; i++) {
        stream_t *stream = state.streams[i];
        struct pollfd *streamfds = stream_fds(stream);

        for (int j = 0; j < stream_nfds(stream); j++) {
            fds[open_fd_count] = streamfds[j];
            fd_to_stream[open_fd_count] = i;
            open_fd_count++;
        }
    }
    assert(fd_count == open_fd_count);

    printf("UIO SND|INFO: starting with %d fds\n", fd_count);

    int expire = 10;

    while (open_fd_count > 0) {
        // We might not need a UIO interrupt for tx/rx, ALSA should signal.
        int ready = poll(fds, open_fd_count, -1);
        if (ready == -1) {
            perror("UIO SND|ERR: Failed to poll descriptors");
            return EXIT_FAILURE;
        }

        bool signal_vmm = false;

        if (fds[UIO_POLLFD].revents & POLLIN) {
            handle_interrupt(&state, &signal_vmm);
        }
        for (int fd_idx = UIO_POLLFD + 1; fd_idx < fd_count; fd_idx++) {
            if (fds[fd_idx].revents & POLLIN) {
                printf("UIO SND|INFO: got poll event on stream %d (idx %d)\n", fd_to_stream[fd_idx],
                       fd_idx);
                // TODO: reenter stream state machine
            }
        }

        if (signal_vmm) {
            signal_ready_to_vmm();
        }

        if (expire-- < 0) break;
    }

    free(fds);
    free(fd_to_stream);
    return EXIT_SUCCESS;
}
