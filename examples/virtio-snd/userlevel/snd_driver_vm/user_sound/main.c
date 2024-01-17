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
}

void fill_pollfds(stream_t **streams, int stream_count, struct pollfd *pollfds)
{
    for (int i = 0; i < stream_count; i++) {
        stream_t *stream = streams[i];
        pollfds[i].fd = stream_res_fd(stream);
        pollfds[i].events = POLLIN;
    }
}

static void handle_response(driver_state_t *state, int resp_fd)
{
    stream_response_t resp;
    int nread = read(resp_fd, &resp, sizeof(resp));
    if (nread != sizeof(resp)) {
        printf("UIO SND|ERR: Failed to read from response queue\n");
        return;
    }

    if (resp.has_tx_free
        && sddf_snd_enqueue_pcm_data(state->rings.tx_free, resp.tx_free.addr, resp.tx_free.len)
            != 0) {
        printf("UIO SND|ERR: failed to enqueue response\n");
    }

    if (sddf_snd_enqueue_response(state->rings.responses, resp.response.cookie,
                                  resp.response.status, resp.response.latency_bytes)
        != 0) {
        printf("UIO SND|ERR: failed to enqueue response\n");
    }
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
                          target_streams[i], state.translate);

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
    int fd_count = state.stream_count + 1;

    struct pollfd *fds = calloc(fd_count, sizeof(struct pollfd));
    if (fds == NULL) {
        perror("UIO SND|ERR: Failed to allocate file descriptor array\n");
        return EXIT_FAILURE;
    }

    fds[UIO_POLLFD].fd = uio_fd;
    fds[UIO_POLLFD].events = POLLIN;
    fds[UIO_POLLFD].revents = 0;

    printf("UIO SND|INFO: UIO fd %d\n", uio_fd);

    fill_pollfds(state.streams, state.stream_count, fds + 1);

    const uint32_t enable = 1;
    if (write(uio_fd, &enable, sizeof(uint32_t)) != sizeof(uint32_t)) {
        perror("UIO SND|ERR: Failed to reenable interrupts\n");
        return EXIT_FAILURE;
    }

    while (true) {
        // printf("UIO SND|INFO: polling with %d fds\n", fd_count);

        int ready = poll(fds, fd_count, -1);
        if (ready == -1) {
            perror("UIO SND|ERR: Failed to poll descriptors");
            return EXIT_FAILURE;
        }

        // printf("UIO SND|INFO: Awake on %d (ready %d) fds: ", fd_count, ready);
        // for (int i = 0; i < fd_count; i++) {
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
            printf("UIO SND|INFO: Got UIO interrupt (new commands!)\n");
            if (read(uio_fd, &irq_count, sizeof(irq_count)) != sizeof(irq_count)) {
                perror("UIO SND|ERR: Failed to read interrupt\n");
                break;
            }

            handle_interrupt(&state);

            if (write(uio_fd, &enable, sizeof(uint32_t)) != sizeof(uint32_t)) {
                perror("UIO SND|ERR: Failed to reenable interrupts\n");
                break;
            }
        }

        bool signal_vmm = false;
        for (int i = 0; i < state.stream_count; i++) {
            if (fds[i + 1].revents & POLLIN) {
                // printf("UIO SND|INFO: Stream %d got response\n", i);
                handle_response(&state, fds[i + 1].fd);
                signal_vmm = true;
            }
        }

        if (signal_vmm) {
            signal_ready_to_vmm(state.signal_addr);
            signal_vmm = false;
        }
    }

    close(signal_fd);
    free(fds);
    return EXIT_SUCCESS;
}
