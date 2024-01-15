#include "sddf_snd_shared_ringbuffer.h"
#include "stream.h"
#include "uio.h"
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

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
    translation_state_t translate;

    bool signal_vmm;
    char *signal_addr;
} driver_state_t;

static void signal_ready_to_vmm(char *signal_addr)
{
    *signal_addr = 1;
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

static char *map_mem(uintptr_t target, int *out_fd)
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        printf("UIO SND|ERR: Failed to open /dev/mem\n");
        return NULL;
    }

    int page_size = getpagesize();

    off_t offset = target & ~(off_t)(page_size - 1);

    *out_fd = fd;
    void *base = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
    if (base == MAP_FAILED) {
        printf("UIO SND|ERR: Failed to map /dev/mem\n");
        return NULL;
    }

    return base + (target & (page_size - 1));
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
    state->rings.commands = (void *)(ring_buffers + RING_BYTES * offset++);
    state->rings.responses = (void *)(ring_buffers + RING_BYTES * offset++);
    state->rings.tx_free = (void *)(ring_buffers + RING_BYTES * offset++);
    state->rings.rx_used = (void *)(ring_buffers + RING_BYTES * offset++);
    state->rings.rx_free = (void *)(ring_buffers + RING_BYTES * offset++);

    state->translate.tx_offset = tx_data - (void *)tx_data_physical;
    state->translate.rx_offset = rx_data - (void *)rx_data_physical;

    return 0;
}

static void handle_cmd(driver_state_t *state, sddf_snd_command_t *cmd)
{
    // printf("UIO SND|INFO: got command %s\n", sddf_snd_command_code_str(cmd->code));

    sddf_snd_status_code_t status = SDDF_SND_S_OK;

    // Check stream ID. Currently switch is redundant but we might want to add
    // jack commands in future.
    switch (cmd->code) {
    case SDDF_SND_CMD_PCM_SET_PARAMS:
    case SDDF_SND_CMD_PCM_PREPARE:
    case SDDF_SND_CMD_PCM_RELEASE:
    case SDDF_SND_CMD_PCM_START:
    case SDDF_SND_CMD_PCM_STOP:
    case SDDF_SND_CMD_PCM_TX:
        if (cmd->stream_id >= state->stream_count) {
            printf("UIO SND|ERR: stream %d does not exist\n", cmd->stream_id);
            status = SDDF_SND_S_BAD_MSG;
            break;
        }

        stream_t *stream = state->streams[cmd->stream_id];
        stream_enqueue_command(stream, cmd);
        break;
    default:
        printf("UIO SND|ERR: unknown command %d\n", cmd->code);
        status = SDDF_SND_S_BAD_MSG;
    }

    if (status != SDDF_SND_S_OK) {
        if (cmd->code == SDDF_SND_CMD_PCM_TX) {
            cmd->pcm.len = SDDF_SND_PCM_BUFFER_SIZE;
            sddf_snd_enqueue_pcm_data(state->rings.tx_free, cmd->pcm.addr, cmd->pcm.len);
        }
        sddf_snd_enqueue_response(state->rings.responses, cmd->cookie, status, 0);
    }
}

static void handle_interrupt(driver_state_t *state)
{
    // printf("UIO SND|INFO: got interrupt\n");

    sddf_snd_command_t cmd;
    while (sddf_snd_dequeue_cmd(state->rings.commands, &cmd) == 0) {
        handle_cmd(state, &cmd);
    }

    for (int i = 0; i < state->stream_count; i++) {
        stream_update(state->streams[i]);
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

            printf("update %d: ", streamfds[j].fd);
            if (streamfds[j].events & POLLIN) printf("in ");
            if (streamfds[j].events & POLLOUT) printf("out ");
            if (streamfds[j].events & POLLPRI) printf("pri ");
            if (streamfds[j].events & POLLERR) printf("err ");
        }
    }
    return open_fd_count;
}

static void respond(uint32_t cookie, sddf_snd_status_code_t status, uint32_t latency_bytes, void *user_data)
{
    driver_state_t *state = user_data;

    if (sddf_snd_enqueue_response(state->rings.responses, cookie, status, latency_bytes) != 0) {
        printf("UIO SND|ERR: failed to enqueue response\n");
    }
    state->signal_vmm = true;
}

static void tx_free(uintptr_t addr, unsigned int len, void *user_data)
{
    driver_state_t *state = user_data;

    if (sddf_snd_enqueue_pcm_data(state->rings.tx_free, addr, len) != 0) {
        printf("UIO SND|ERR: failed to enqueue response\n");
    }
    state->signal_vmm = true;
}

int main(void)
{
    int err = system("alsactl init -U");
    // assert(!err);

    int uio_fd = open("/dev/uio0", O_RDWR);
    if (uio_fd == -1) {
        perror("Error opening /dev/uio0");
        return EXIT_FAILURE;
    }
    printf("UIO SND|INFO: opened /dev/uio0\n");

    driver_state_t state;
    err = init_mappings(&state, uio_fd);
    if (err) {
        printf("UIO SND|ERR: failed to initialise mappings\n");
        return EXIT_FAILURE;
    }

    int signal_fd;
    state.signal_addr = map_mem(UIO_SND_FAULT_ADDRESS, &signal_fd);
    if (state.signal_addr == NULL) {
        return EXIT_FAILURE;
    }
    printf("UIO SND|INFO: opened /dev/mem\n");

    // The idea is that this should work even if one stream fails to open.
    snd_pcm_stream_t target_streams[MAX_STREAMS] = {
        SND_PCM_STREAM_PLAYBACK,
        SND_PCM_STREAM_CAPTURE,
    };

    state.stream_count = 0;

    for (int i = 0; i < MAX_STREAMS; i++) {
        state.streams[state.stream_count]
            = stream_open(&state.shared_state->stream_info[state.stream_count], SOUND_DEVICE,
                          target_streams[i], state.translate, respond, tx_free, &state);

        if (state.streams[state.stream_count] == NULL)
            printf("Failed to initialise target stream %d\n", i);
        else
            state.stream_count++;
    }

    state.shared_state->streams = state.stream_count;
    printf("UIO SND|INFO: %d streams available\n", state.stream_count);

    // Signal ready even if we don't create all streams.
    signal_ready_to_vmm(state.signal_addr);

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

    printf("UIO SND|INFO: UIO fd %d\n", uio_fd);

    int curr_fd_count
        = update_pollfds(state.streams, state.stream_count, fds, fd_to_stream, stream_to_fds);

    const uint32_t enable = 1;
    if (write(uio_fd, &enable, sizeof(uint32_t)) != sizeof(uint32_t)) {
        perror("UIO SND|ERR: Failed to reenable interrupts\n");
        return EXIT_FAILURE;
    }
    
    while (true) {
        // printf("UIO SND|INFO: polling with %d fds\n", curr_fd_count);

        // We might not need a UIO interrupt for tx/rx, ALSA should signal.
        int ready = poll(fds, curr_fd_count, -1);
        if (ready == -1) {
            perror("UIO SND|ERR: Failed to poll descriptors");
            return EXIT_FAILURE;
        }

        // printf("UIO SND|INFO: Awake on %d (%d) fds: ", curr_fd_count, ready);
        // for (int i = 0; i < curr_fd_count; i++) {
        //     char *state = "off";
        //     if (fds[i].revents & POLLIN) state = "in";
        //     else if (fds[i].revents & POLLOUT) state = "out";
        //     else if (fds[i].revents & POLLPRI) state = "pri";
        //     else if (fds[i].revents & POLLERR) state = "err";
        //     printf("%d: %s, ", fds[i].fd, state);
        // }
        // putchar('\n');

        if (fds[UIO_POLLFD].revents & POLLIN) {
            int32_t irq_count;
            printf("Reading interrupt\n");
            if(read(uio_fd, &irq_count, sizeof(irq_count)) != sizeof(irq_count)) {
                perror("UIO SND|ERR: Failed to read interrupt\n");
                break;
            }
            printf("Got %d interrupts\n", irq_count);

            handle_interrupt(&state);

            printf("Writing interrupt\n");
            if (write(uio_fd, &enable, sizeof(uint32_t)) != sizeof(uint32_t)) {
                perror("UIO SND|ERR: Failed to reenable interrupts\n");
                break;
            }
        }

        for (int i = 0; i < state.stream_count; i++) {
            if (stream_to_fds[i] == -1) {
                continue;
            }

            stream_t *stream = state.streams[i];
            int fd_begin = stream_to_fds[i];
            int fd_end = fd_begin + stream_nfds(stream);

            // snd_pcm_stream_t direction = stream_direction(stream);
            // short target_event = direction == SND_PCM_STREAM_PLAYBACK ? POLLOUT : POLLIN; 

            unsigned short revents;
            stream_demangle_fds(stream, &fds[fd_begin], stream_nfds(stream), &revents);

            for (int fd_index = fd_begin; fd_index < fd_end; fd_index++) {
                // if (fds[fd_index].revents & target_event) {
                if (fds[fd_index].revents) {
                    // printf("UIO SND|INFO: got poll event on stream %d (idx %d)\n", i, fd_index);
                    // this assumes driver will signal if there are any commands
                    stream_update(stream);
                    break;
                }
            }
        }

        if (state.signal_vmm) {
            signal_ready_to_vmm(state.signal_addr);
            state.signal_vmm = false;
        }

        curr_fd_count
            = update_pollfds(state.streams, state.stream_count, fds, fd_to_stream, stream_to_fds);
    }

    for (int i = 0; i < state.stream_count; i++) {
        stream_close(state.streams[i]);
    }

    close(signal_fd);
    free(fds);
    free(fd_to_stream);
    free(stream_to_fds);
    return EXIT_SUCCESS;
}
