#include "sddf_snd_shared_ringbuffer.h"
#include "stream.h"
#include "uio.h"
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

static ssize_t translate_offset(translation_state_t *translate, snd_pcm_stream_t direction)
{
    switch (direction) {
    case SND_PCM_STREAM_PLAYBACK:
        return translate->tx_offset;
    case SND_PCM_STREAM_CAPTURE:
        return translate->rx_offset;
    }
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
    state->rings.tx_req = (void *)(ring_buffers + RING_BYTES * offset++);
    state->rings.tx_res = (void *)(ring_buffers + RING_BYTES * offset++);
    state->rings.rx_res = (void *)(ring_buffers + RING_BYTES * offset++);
    state->rings.rx_req = (void *)(ring_buffers + RING_BYTES * offset++);

    state->translate.tx_offset = tx_data - (void *)tx_data_physical;
    state->translate.rx_offset = rx_data - (void *)rx_data_physical;

    return 0;
}

static void fail_pcm(sddf_snd_pcm_data_ring_t *ring, sddf_snd_pcm_data_t *pcm)
{
    pcm->status = SDDF_SND_S_BAD_MSG;
    pcm->latency_bytes = 0;
    if (sddf_snd_enqueue_pcm_data(ring, pcm) != 0) {
        printf("UIO SND|ERR: Failed to send fail response\n");
    }
}

static bool handle_pcm_request(driver_state_t *state,
                              sddf_snd_pcm_data_ring_t *res_ring,
                              sddf_snd_pcm_data_t *pcm,
                              snd_pcm_stream_t expected_direction)
{
    if (pcm->stream_id >= state->stream_count) {
        printf("UIO SND|ERR: Invalid tx request\n");
        fail_pcm(res_ring, pcm);
        return false;
    }
    stream_t *stream = state->streams[pcm->stream_id];
    if (stream_direction(stream) != expected_direction) {
        printf("UIO SND|ERR: Failed to enqueue tx_used to stream\n");
        fail_pcm(res_ring, pcm);
        return false;
    }
    if (stream_enqueue_pcm_req(stream, pcm) != 0) {
        printf("UIO SND|ERR: Failed to enqueue tx_used to stream\n");
        return false;
    }
    return true;
}

static void handle_uio_interrupt(driver_state_t *state)
{
    sddf_snd_command_t cmd;
    sddf_snd_pcm_data_t pcm;
    bool notify[MAX_STREAMS] = { false };

    while (sddf_snd_dequeue_cmd(state->rings.commands, &cmd) == 0) {
        if (cmd.stream_id >= state->stream_count) {
            printf("UIO SND|ERR: Invalid stream id\n");
            continue;
        }
        if (stream_enqueue_command(state->streams[cmd.stream_id], &cmd) != 0) {
            printf("UIO SND|ERR: Failed to enqueue command to stream\n");
            break;
        }
        notify[cmd.stream_id] = true;
    }

    while (sddf_snd_dequeue_pcm_data(state->rings.tx_req, &pcm) == 0) {
        if (!handle_pcm_request(state, state->rings.tx_res, &pcm, SND_PCM_STREAM_PLAYBACK)) {
            break;
        }
    }

    while (sddf_snd_dequeue_pcm_data(state->rings.rx_req, &pcm) == 0) {
        if (!handle_pcm_request(state, state->rings.rx_res, &pcm, SND_PCM_STREAM_CAPTURE)) {
            break;
        }
    }

    // Avoid notifying a stream more than necessary.
    for (int i = 0; i < state->stream_count; i++) {
        if (notify[i]) {
            stream_notify(state->streams[i]);
        }
    }
}

void fill_pollfds(stream_t **streams, int stream_count, struct pollfd *pollfds)
{
    for (int i = 0; i < stream_count; i++) {
        stream_t *stream = streams[i];
        pollfds[i].fd = stream_response_fd(stream);
        pollfds[i].events = POLLIN;
    }
}

static void handle_stream_notify(driver_state_t *state, stream_t *stream)
{
    sddf_snd_response_t response;
    sddf_snd_pcm_data_t pcm;

    while (stream_dequeue_response(stream, &response) == 0) {
        if (sddf_snd_enqueue_response(state->rings.responses, &response) != 0) {
            printf("UIO SND|ERR: failed to enqueue response\n");
            break;
        }
    }

    sddf_snd_pcm_data_ring_t *dest;
    switch (stream_direction(stream)) {
    case SND_PCM_STREAM_PLAYBACK:
        dest = state->rings.tx_res;
        break;
    case SND_PCM_STREAM_CAPTURE:
        dest = state->rings.rx_res;
        break;
    }

    while (stream_dequeue_pcm_res(stream, &pcm) == 0) {
        if (sddf_snd_enqueue_pcm_data(dest, &pcm) != 0) {
            printf("UIO SND|ERR: failed to enqueue pcm response\n");
            break;
        }
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
                          target_streams[i], translate_offset(&state.translate, target_streams[i]));

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
        int ready = poll(fds, fd_count, -1);
        if (ready == -1) {
            perror("UIO SND|ERR: Failed to poll descriptors");
            return EXIT_FAILURE;
        }

        if (fds[UIO_POLLFD].revents & POLLIN) {
            int32_t irq_count;
            // printf("UIO SND|INFO: Got UIO interrupt (new commands!)\n");
            if (read(uio_fd, &irq_count, sizeof(irq_count)) != sizeof(irq_count)) {
                perror("UIO SND|ERR: Failed to read interrupt\n");
                break;
            }
            if (write(uio_fd, &enable, sizeof(uint32_t)) != sizeof(uint32_t)) {
                perror("UIO SND|ERR: Failed to reenable interrupts\n");
                break;
            }

            handle_uio_interrupt(&state);
        }

        bool signal_vmm = false;
        for (int i = 1; i <= state.stream_count; i++) {
            if (fds[i].revents & POLLIN) {

                uint64_t message;
                int nread = read(fds[i].fd, &message, sizeof(message));

                if (nread != sizeof(message)) {
                    printf("UIO SND|ERR: Failed to read stream eventfd\n");
                    continue;
                }

                handle_stream_notify(&state, state.streams[i - 1]);
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
