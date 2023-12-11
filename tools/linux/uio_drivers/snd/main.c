#include "log.h"
#include "stream.h"
#include "uio_fault_addr.h"
#include <sddf/sound/queue.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGE_SIZE_4K 0x1000
#define RING_BYTES 0x200000

#define DEFAULT_DEVICE "default"
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
#define PCM_DATA_SLOT 2
#define PCM_DATA_ADDR UIO_ADDR("2")
#define PCM_DATA_SIZE UIO_SIZE("2")

typedef struct driver_state {
    stream_t *streams[MAX_STREAMS];
    int stream_count;

    sound_shared_state_t *shared_state;
    sound_queues_t rings;
    ssize_t translate;

    char *signal_addr;
} driver_state_t;

static void signal_ready_to_vmm(char *signal_addr)
{
    *signal_addr = 1;
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
        LOG_SOUND_ERR("Size string is too long to fit in size_str buffer\n");
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
        LOG_SOUND_ERR("Failed to get value\n");
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
        LOG_SOUND_ERR("Failed to open /dev/mem\n");
        return NULL;
    }

    int page_size = getpagesize();

    off_t offset = target & ~(off_t)(page_size - 1);

    *out_fd = fd;
    void *base = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
    if (base == MAP_FAILED) {
        LOG_SOUND_ERR("Failed to map /dev/mem\n");
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
    void *pcm = map_uio(PCM_DATA_SLOT, PCM_DATA_SIZE, uio_fd);
    if (pcm == MAP_FAILED) {
        perror("Error mapping pcm");
        return -1;
    }

    size_t pcm_physical;

    err = get_uio_map_value(PCM_DATA_ADDR, &pcm_physical);
    if (err) {
        perror("Error getting pcm_physical");
        return -1;
    }

    int offset = 0;
    state->rings.cmd_req = (void *)(ring_buffers + RING_BYTES * offset++);
    state->rings.cmd_res = (void *)(ring_buffers + RING_BYTES * offset++);
    state->rings.pcm_req = (void *)(ring_buffers + RING_BYTES * offset++);
    state->rings.pcm_res = (void *)(ring_buffers + RING_BYTES * offset++);

    state->translate = pcm - (void *)pcm_physical;

    return 0;
}

static void fail_cmd(sound_cmd_queue_t *ring, sound_cmd_t *cmd)
{
    cmd->status = SOUND_S_BAD_MSG;
    if (sound_enqueue_cmd(ring, cmd) != 0) {
        LOG_SOUND_ERR("Failed to send fail response\n");
    }
}

static void fail_pcm(sound_pcm_queue_t *ring, sound_pcm_t *pcm)
{
    pcm->status = SOUND_S_BAD_MSG;
    pcm->latency_bytes = 0;
    if (sound_enqueue_pcm(ring, pcm) != 0) {
        LOG_SOUND_ERR("Failed to send fail response\n");
    }
}

static bool handle_uio_interrupt(driver_state_t *state)
{
    sound_cmd_t cmd;
    sound_pcm_t pcm;

    while (sound_dequeue_cmd(state->rings.cmd_req, &cmd) == 0) {
        if (cmd.stream_id >= state->stream_count) {
            LOG_SOUND_ERR("Invalid stream id\n");
            fail_cmd(state->rings.cmd_res, &cmd);
            continue;
        }
        stream_enqueue_command(state->streams[cmd.stream_id], &cmd);
    }

    while (sound_dequeue_pcm(state->rings.pcm_req, &pcm) == 0) {
        if (pcm.stream_id >= state->stream_count) {
            LOG_SOUND_ERR("Invalid stream id\n");
            fail_pcm(state->rings.pcm_res, &pcm);
            continue;
        }
        stream_enqueue_pcm_req(state->streams[pcm.stream_id], &pcm);
    }

    bool notify_client = false;
    for (int i = 0; i < state->stream_count; i++) {
        if (stream_update(state->streams[i])) {
            notify_client = true;
        }
    }
    return notify_client;
}

int main(int argc, char **argv)
{
    system("alsactl init -U");

    LOG_SOUND("Starting sound driver\n");

    int uio_fd = open("/dev/uio0", O_RDWR);
    if (uio_fd == -1) {
        LOG_SOUND_ERR("Error opening /dev/uio0\n");
        return EXIT_FAILURE;
    }
    LOG_SOUND("Opened /dev/uio0\n");

    driver_state_t state;
    memset(&state, 0, sizeof(state));

    int err = init_mappings(&state, uio_fd);
    if (err) {
        printf("UIO SND|ERR: failed to initialise mappings\n");
        return EXIT_FAILURE;
    }

    int signal_fd;
    state.signal_addr = map_mem(UIO_SND_FAULT_ADDRESS, &signal_fd);
    if (state.signal_addr == NULL) {
        return EXIT_FAILURE;
    }
    LOG_SOUND("Opened /dev/mem\n");

    // The idea is that this should work even if one stream fails to open.
    snd_pcm_stream_t stream_directions[MAX_STREAMS] = {
        SND_PCM_STREAM_PLAYBACK,
        SND_PCM_STREAM_CAPTURE,
    };

    bool stream_ready[MAX_STREAMS];
    memset(stream_ready, 0, sizeof(bool) * MAX_STREAMS);

    state.stream_count = 0;

    int tries = 0;
    while (state.stream_count != MAX_STREAMS && tries < 10) {
        for (int i = 0; i < MAX_STREAMS; i++) {

            snd_pcm_stream_t direction = stream_directions[i];
            if (stream_ready[i]) {
                continue;
            }

            char *device_name;
            if (argc - 1 > i) {
                device_name = argv[i+1];
            } else {
                device_name = DEFAULT_DEVICE;
            }

            state.streams[state.stream_count] = stream_open(
                &state.shared_state->stream_info[state.stream_count], device_name, direction,
                state.translate, state.rings.cmd_res, state.rings.pcm_res);

            if (state.streams[state.stream_count] == NULL) {
                LOG_SOUND_WARN("Could not initialise target stream %d (%s)\n", i, device_name);
            } else {
                LOG_SOUND("Initialised stream %d (%s)\n", i, device_name);
                stream_ready[i] = true;
                state.stream_count++;
            }
        }

        if (state.stream_count != MAX_STREAMS) {
            LOG_SOUND_WARN("Trying again in 1s...\n");
            sleep(1);
            tries++;
        }
    }

    state.shared_state->streams = state.stream_count;
    LOG_SOUND("Initialised %d streams\n", state.stream_count);

    // Signal ready even if we don't create all streams.
    signal_ready_to_vmm(state.signal_addr);

    if (state.stream_count == 0) {
        LOG_SOUND_ERR("No streams available, exiting\n");
        return EXIT_FAILURE;
    }

    // Start with 1 for UIO fd
    int fd_count = state.stream_count + 1;

    struct pollfd *fds = calloc(fd_count, sizeof(struct pollfd));
    if (fds == NULL) {
        LOG_SOUND_ERR("Failed to allocate file descriptor array\n");
        return EXIT_FAILURE;
    }

    fds[UIO_POLLFD].fd = uio_fd;
    fds[UIO_POLLFD].events = POLLIN;
    fds[UIO_POLLFD].revents = 0;

    for (int i = 0; i < state.stream_count; i++) {
        stream_t *stream = state.streams[i];
        fds[i + 1].fd = stream_timer_fd(stream);
        fds[i + 1].events = POLLIN;
    }

    const uint32_t enable = 1;
    if (write(uio_fd, &enable, sizeof(uint32_t)) != sizeof(uint32_t)) {
        LOG_SOUND_ERR("Failed to reenable interrupts\n");
        return EXIT_FAILURE;
    }

    while (true) {
        bool signal_vmm = false;

        int ready = poll(fds, fd_count, -1);
        if (ready == -1) {
            LOG_SOUND_ERR("Failed to poll descriptors\n");
            return EXIT_FAILURE;
        }

        if (fds[UIO_POLLFD].revents & POLLIN) {
            int32_t irq_count;
            if (read(uio_fd, &irq_count, sizeof(irq_count)) != sizeof(irq_count)) {
                LOG_SOUND_ERR("Failed to read interrupt\n");
                break;
            }
            if (write(uio_fd, &enable, sizeof(uint32_t)) != sizeof(uint32_t)) {
                LOG_SOUND_ERR("Failed to reenable interrupts\n");
                break;
            }

            if (handle_uio_interrupt(&state)) {
                signal_vmm = true;
            }
        }

        for (int i = 1; i <= state.stream_count; i++) {
            if (fds[i].revents & POLLIN) {

                uint64_t message;
                int nread = read(fds[i].fd, &message, sizeof(message));

                if (nread != sizeof(message)) {
                    LOG_SOUND_ERR("Failed to read stream eventfd\n");
                    continue;
                }

                if (stream_update(state.streams[i - 1])) {
                    signal_vmm = true;
                }
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
