#include "stream.h"
#include "convert.h"
#include "queue.h"
#include <alsa/pcm.h>
#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

// This queue may have many TX buffers along side commands.
#define PLAYBACK_QUEUE_SIZE (SDDF_SND_NUM_BUFFERS / 4)
#define PCM_QUEUE_SIZE 8

// 0.5 seconds
#define LATENCY 500000
// Time between hardware interrupts
#define PERIOD_TIME 100000

#define BITS_PER_BYTE 8

#define COMMAND_FD 0
#define TIMER_FD 1

#define NS_PER_SECOND 1000000000

typedef enum stream_state {
    STREAM_STATE_UNSET,
    STREAM_STATE_SET,
    STREAM_STATE_PAUSED,
    STREAM_STATE_PLAYING,
    STREAM_STATE_IO_ERR,
    STREAM_STATE_DRAINING,
} stream_state_t;

struct stream {
    pthread_t thread;

    // ALSA
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
    snd_pcm_stream_t direction;

    // Stream state
    stream_state_t state;
    translation_state_t translate;

    int frame_size;
    snd_pcm_sframes_t buffer_size;
    snd_pcm_sframes_t period_size;
    snd_pcm_sframes_t start_threshold;
    unsigned rate;

    pcm_queue_t *played_frames;
    snd_pcm_sframes_t frame_offset;
    int drain_count;

    bool timer_enabled;
    int timer_fd;

    // Communication
    int command_fd;
    int response_fd;

    sddf_snd_cmd_ring_t commands;
    sddf_snd_response_ring_t responses;

    sddf_snd_pcm_data_ring_t tx_used;
    sddf_snd_pcm_data_ring_t tx_free;

    sddf_snd_pcm_data_ring_t rx_used;
    sddf_snd_pcm_data_ring_t rx_free;
};

struct alsa_params {
    unsigned resample;
    snd_pcm_format_t format;
    unsigned channels;
    unsigned rate;
    unsigned latency_us;
    unsigned period_us; // time between hardware interrupts
};

struct buffer_state {
    snd_pcm_sframes_t buffer_size;
    snd_pcm_sframes_t period_size;
};

#define ZERO_COUNT 4096
char zeroes[ZERO_COUNT];

static const char *stream_state_str(stream_state_t state);
static char *snd_state_str(snd_pcm_t *handle);

static void print_err(int err, const char *msg)
{
    fprintf(stderr, "UIO SND|ERR: %s: %s\n", msg, snd_strerror(err));
}

static snd_pcm_sframes_t min(snd_pcm_sframes_t a, snd_pcm_sframes_t b)
{
    return a <= b ? a : b;
}

static void *translate_tx(translation_state_t *translate, void *phys_addr)
{
    return phys_addr + translate->tx_offset;
}

static void *translate_rx(translation_state_t *translate, void *phys_addr)
{
    return phys_addr + translate->rx_offset;
}

static void print_bytes(const void *data)
{
    for (int i = 0; i < 16; i++) {
        printf("%02x ", ((uint8_t *)data)[i]);
    }
    printf("\n");
}

static snd_pcm_sframes_t stream_write(stream_t *stream, const void *pcm_data, snd_pcm_sframes_t to_write)
{
    snd_pcm_sframes_t offset = 0;

    while (to_write > 0) {
        snd_pcm_sframes_t written = snd_pcm_writei(stream->handle, pcm_data + offset * stream->frame_size, to_write);

        if (written == -EAGAIN) {
            return offset;
        } else if (written < 0) {
            printf("UIO SND|ERR: failed to flush pcm: %s, state %s\n", snd_strerror(written),
                   snd_state_str(stream->handle));

            stream->state = STREAM_STATE_IO_ERR;
            return offset;
        }

        if (written < to_write) {
            printf("UIO SND|INFO: Warning: tried to write %ld but wrote %ld\n", to_write, written);
        }

        // TODO: remove
        // if (pcm_data != zeroes) {
        //     print_bytes(pcm_data);
        // }

        to_write -= written;
        offset += written;
    }
    return offset;
}

static bool send_response(stream_t *stream)
{
    sddf_snd_pcm_data_t *response = pcm_queue_front(stream->played_frames);
    if (!response) {
        return false;
    }

    response->latency_bytes = stream->buffer_size;
    response->status = stream->state == STREAM_STATE_IO_ERR ? SDDF_SND_S_IO_ERR : SDDF_SND_S_OK;
    response->len = SDDF_SND_PCM_BUFFER_SIZE;

    if (sddf_snd_enqueue_pcm_data(&stream->tx_free, response) != 0) {
        printf("UIO SND|INFO: Failed to enqueue tx_free\n");
        return false;
    }
    pcm_queue_dequeue(stream->played_frames);
    return true;
}

static void notify_main(stream_t *stream)
{
    uint64_t message = 1;
    int nwritten = write(stream->response_fd, &message, sizeof(message));
    if (nwritten != sizeof(message)) {
        printf("UIO SND|ERR: Failed to send response notification\n");
    }
}

static int flush_playback(stream_t *stream, int max_count)
{
    int response_count = 0;

    snd_pcm_sframes_t avail = snd_pcm_avail(stream->handle);

    if (avail < 0) {
        printf("Failed to get avail: %s\n", snd_strerror(avail));
        return response_count;
    }

    while (!sddf_snd_ring_empty(&stream->tx_used.state) && max_count-- > 0) {

        sddf_snd_pcm_data_t *pcm = sddf_snd_pcm_data_front(&stream->tx_used);

        snd_pcm_sframes_t pcm_frames = pcm->len / stream->frame_size;
        snd_pcm_sframes_t remaining = pcm_frames - stream->frame_offset;
        snd_pcm_sframes_t to_write = min(remaining, avail);

        void *addr_offset = (void *)pcm->addr + (stream->frame_offset * stream->frame_size);
        const void *pcm_data = translate_tx(&stream->translate, addr_offset);

        snd_pcm_sframes_t transmitted = stream_write(stream, pcm_data, to_write);

        if (transmitted < to_write) {
            printf("UIO SND|ERR: Failed to write whole tx\n");
            break;
        }

        avail -= transmitted;
        stream->frame_offset += transmitted;

        if (stream->frame_offset == pcm_frames) {

            pcm_queue_enqueue(stream->played_frames, pcm);

            if (sddf_snd_ring_dequeue(&stream->tx_used.state) != 0) {
                printf("UIO SND|ERR: Failed to dequeue");
                break;
            }

            if (!send_response(stream)) {
                printf("UIO SND|INFO: Failed to send response\n");
            }
            notify_main(stream);
            response_count++;

            stream->frame_offset = 0;

        }
        if (avail <= 0) {
            break;
        }
    }

    return response_count;
}

static sddf_snd_status_code_t set_hwparams(snd_pcm_t *handle, snd_pcm_hw_params_t *hw_params,
                                           snd_pcm_access_t access, struct alsa_params *params,
                                           struct buffer_state *state)
{
    unsigned int rrate;
    snd_pcm_uframes_t size;
    int err, dir;

    /* choose all parameters */
    err = snd_pcm_hw_params_any(handle, hw_params);
    if (err < 0) {
        printf("Broken configuration for playback: no configurations available: %s\n",
               snd_strerror(err));
        return SDDF_SND_S_IO_ERR;
    }
    /* set hardware resampling */
    err = snd_pcm_hw_params_set_rate_resample(handle, hw_params, params->resample);
    if (err < 0) {
        printf("Resampling setup failed for playback: %s\n", snd_strerror(err));
        return SDDF_SND_S_NOT_SUPP;
    }
    /* set the interleaved read/write format */
    err = snd_pcm_hw_params_set_access(handle, hw_params, access);
    if (err < 0) {
        printf("Access type not available for playback: %s\n", snd_strerror(err));
        return SDDF_SND_S_NOT_SUPP;
    }
    /* set the sample format */
    err = snd_pcm_hw_params_set_format(handle, hw_params, params->format);
    if (err < 0) {
        printf("Sample format not available for playback: %s\n", snd_strerror(err));
        return SDDF_SND_S_NOT_SUPP;
    }
    /* set the count of channels */
    err = snd_pcm_hw_params_set_channels(handle, hw_params, params->channels);
    if (err < 0) {
        printf("Channels count (%u) not available for playbacks: %s\n", params->channels,
               snd_strerror(err));
        return SDDF_SND_S_NOT_SUPP;
    }
    /* set the stream rate */
    rrate = params->rate;
    err = snd_pcm_hw_params_set_rate_near(handle, hw_params, &rrate, 0);
    if (err < 0) {
        printf("Rate %uHz not available for playback: %s\n", params->rate, snd_strerror(err));
        return SDDF_SND_S_NOT_SUPP;
    }
    if (rrate != params->rate) {
        printf("Rate doesn't match (requested %uHz, get %iHz)\n", params->rate, err);
        return SDDF_SND_S_NOT_SUPP;
    }
    /* set the buffer time */
    err = snd_pcm_hw_params_set_buffer_time_near(handle, hw_params, &params->latency_us, &dir);
    if (err < 0) {
        printf("Unable to set buffer time %u for playback: %s\n", params->latency_us,
               snd_strerror(err));
        return SDDF_SND_S_IO_ERR;
    }
    err = snd_pcm_hw_params_get_buffer_size(hw_params, &size);
    if (err < 0) {
        printf("Unable to get buffer size for playback: %s\n", snd_strerror(err));
        return SDDF_SND_S_IO_ERR;
    }
    state->buffer_size = size;

    /* set the period time -- interval between consecutive interrupts from the sound card */
    err = snd_pcm_hw_params_set_period_time_near(handle, hw_params, &params->period_us, &dir);
    if (err < 0) {
        printf("Unable to set period time %u for playback: %s\n", params->period_us,
               snd_strerror(err));
        return SDDF_SND_S_IO_ERR;
    }
    // err = snd_pcm_hw_params_set_period_size(handle, hw_params, params->period_bytes, 0);
    err = snd_pcm_hw_params_get_period_size(hw_params, &size, &dir);
    if (err < 0) {
        printf("Unable to set period size for playback: %s\n", snd_strerror(err));
        return SDDF_SND_S_IO_ERR;
    }
    // state->period_size = params->period_bytes;
    state->period_size = size;
    /* write the parameters to device */
    err = snd_pcm_hw_params(handle, hw_params);
    if (err < 0) {
        printf("Unable to set hw params for playback: %s\n", snd_strerror(err));
        return SDDF_SND_S_NOT_SUPP;
    }
    return 0;
}

static sddf_snd_status_code_t set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams,
                                           struct buffer_state *state,
                                           snd_pcm_sframes_t *start_threshold)
{
    int err;

    /* get the current swparams */
    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0) {
        printf("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* start the transfer when the buffer is almost full: */
    /* (buffer_size / avail_min) * avail_min */
    unsigned threshold = (state->buffer_size / state->period_size) * state->period_size;
    printf("UIO SND|INFO: Setting start threshold to %u/%lu\n", threshold, state->buffer_size);

    err = snd_pcm_sw_params_set_start_threshold(handle, swparams, threshold);
    if (err < 0) {
        printf("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
        return err;
    }
    *start_threshold = (snd_pcm_sframes_t)threshold;

    /* allow the transfer when at least period_size samples can be processed */
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, state->period_size);
    if (err < 0) {
        printf("Unable to set avail min for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* write the parameters to the playback device */
    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0) {
        printf("Unable to set sw params for playback: %s\n", snd_strerror(err));
        return err;
    }
    return 0;
}

static sddf_snd_status_code_t stream_set_params(stream_t *stream, sddf_snd_pcm_set_params_t *params)
{
    printf("UIO SND|INFO: set params stream %p, format %s, rate %u, channels %d, period_bytes %u, "
           "buffer_bytes %u\n",
           stream, sddf_snd_pcm_fmt_str(params->format), sddf_rate_to_alsa(params->rate),
           params->channels, params->period_bytes, params->buffer_bytes);

    bool state_valid = stream->state == STREAM_STATE_UNSET || stream->state == STREAM_STATE_SET
        || stream->state == STREAM_STATE_PAUSED;
    if (!state_valid) {
        printf("UIO SND|ERR: Cannot set params, invalid state\n");
        return SDDF_SND_S_BAD_MSG;
    }

    snd_pcm_t *handle = stream->handle;

    // Set the sample format
    snd_pcm_format_t format = sddf_format_to_alsa(params->format);
    if (format == SND_PCM_FORMAT_UNKNOWN) {
        printf("UIO SND|ERR: Invalid format %d\n", params->format);
        return SDDF_SND_S_NOT_SUPP;
    }

    unsigned rate = sddf_rate_to_alsa(params->rate);
    if (rate == INVALID_RATE) {
        printf("UIO SND|ERR: Invalid rate %d\n", params->rate);
        return SDDF_SND_S_NOT_SUPP;
    }

    struct alsa_params alsa_params;
    alsa_params.channels = params->channels;
    alsa_params.format = format;
    alsa_params.latency_us = LATENCY;
    alsa_params.period_us = PERIOD_TIME;
    // alsa_params.period_bytes = params->period_bytes;
    alsa_params.rate = rate;
    alsa_params.resample = 1;

    struct buffer_state buffer_state;
    sddf_snd_status_code_t code;

    code = set_hwparams(handle, stream->hw_params, SND_PCM_ACCESS_RW_INTERLEAVED, &alsa_params,
                        &buffer_state);

    if (code != SDDF_SND_S_OK) {
        printf("UIO SND|ERR: failed to set hardware params\n");
        return code;
    }

    snd_pcm_sframes_t start_threshold;
    code = set_swparams(handle, stream->sw_params, &buffer_state, &start_threshold);

    if (code != SDDF_SND_S_OK) {
        printf("UIO SND|ERR: failed to set software params\n");
        return code;
    }

    stream->state = STREAM_STATE_SET;
    stream->frame_size = (snd_pcm_format_physical_width(format) / BITS_PER_BYTE) * params->channels;
    stream->buffer_size = buffer_state.buffer_size;
    stream->period_size = buffer_state.period_size;
    stream->start_threshold = start_threshold;
    stream->rate = rate;

    printf("UIO SND|INFO: frame size %d\n", stream->frame_size);
    printf("UIO SND|INFO: client period_bytes %d, driver period_size %ld, driver buffer_size %ld\n",
           params->period_bytes, stream->period_size * stream->frame_size,
           stream->buffer_size * stream->frame_size);

    return SDDF_SND_S_OK;
}

static sddf_snd_status_code_t stream_prepare(stream_t *stream)
{
    if (stream->state != STREAM_STATE_SET && stream->state != STREAM_STATE_PAUSED) {
        printf("UIO SND|ERR: Cannot prepare in state '%s'\n", stream_state_str(stream->state));
        return SDDF_SND_S_BAD_MSG;
    }

    int err = snd_pcm_prepare(stream->handle);
    if (err) {
        printf("UIO SND|ERR: Failed to prepare stream: %s\n", snd_strerror(err));
        return SDDF_SND_S_IO_ERR;
    }

    printf("UIO SND|INFO: Prepared stream\n");
    stream->state = STREAM_STATE_PAUSED;

    return SDDF_SND_S_OK;
}

static sddf_snd_status_code_t stream_release(stream_t *stream)
{
    if (stream->state != STREAM_STATE_PAUSED && stream->state != STREAM_STATE_IO_ERR) {
        printf("UIO SND|ERR: Cannot release stream now, not paused\n");
        return SDDF_SND_S_BAD_MSG;
    }

    printf("UIO SND|INFO: Released stream\n");
    stream->state = STREAM_STATE_SET;

    return SDDF_SND_S_OK;
}

static sddf_snd_status_code_t stream_start(stream_t *stream)
{
    if (stream->state != STREAM_STATE_PAUSED) {
        printf("UIO SND|ERR: Cannot start stream now, invalid state\n");
        return SDDF_SND_S_BAD_MSG;
    }

    printf("UIO SND|INFO: Starting stream\n");
    stream->state = STREAM_STATE_PLAYING;

    // Repeat every period, start immediately.
    struct itimerspec timer;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_nsec = (NS_PER_SECOND / (long)stream->rate) * stream->period_size * 3 / 2;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_nsec = 1;
    if (timerfd_settime(stream->timer_fd, 0, &timer, NULL) != 0) {
        printf("UIO SND: ERR: Failed to set period timer\n");
    }
    stream->timer_enabled = true;

    // Drop any TX frames sent before START.
    sddf_snd_pcm_data_t pcm;
    while (sddf_snd_dequeue_pcm_data(&stream->tx_used, &pcm) == 0) {
        pcm_queue_enqueue(stream->played_frames, &pcm);
    }

    return SDDF_SND_S_OK;
}

static sddf_snd_status_code_t stream_stop(stream_t *stream, bool *blocked)
{
    bool state_valid =
        stream->state == STREAM_STATE_PLAYING ||
        stream->state == STREAM_STATE_DRAINING ||
        stream->state == STREAM_STATE_IO_ERR;

    if (!state_valid) {
        printf("UIO SND|ERR: Cannot stop stream at %s, invalid state\n",
            stream_state_str(stream->state));

        return SDDF_SND_S_BAD_MSG;
    }

    // Initiate stop
    if (stream->state == STREAM_STATE_PLAYING) {
        stream->drain_count = sddf_snd_ring_size(&stream->tx_used.state);
        stream->state = STREAM_STATE_DRAINING;
    }

    stream->drain_count -= flush_playback(stream, stream->drain_count);
    assert(stream->drain_count >= 0);

    if (stream->drain_count > 0) {
        printf("UIO SND|INFO: Stopping stream\n");
        *blocked = true;
        return SDDF_SND_S_OK;
    }

    // Flush remaining responses gradually
    send_response(stream);

    int err = snd_pcm_drain(stream->handle);

    if (err == -EAGAIN) {
        *blocked = true;
        return SDDF_SND_S_OK;

    } else if (err < 0) {
        printf("UIO SND|ERR: Failed to drain stream: %s\n", snd_strerror(err));
        return SDDF_SND_S_IO_ERR;
    } else {
        // Flush remaining responses immediately
        while (send_response(stream));

        // Finished drain, disable timer.
        struct itimerspec timer;
        timer.it_interval.tv_sec = 0;
        timer.it_interval.tv_nsec = 0;
        timer.it_value.tv_sec = 0;
        timer.it_value.tv_nsec = 0;
        if (timerfd_settime(stream->timer_fd, 0, &timer, NULL) != 0) {
            printf("UIO SND: ERR: Failed to set period timer\n");
        }
        stream->timer_enabled = false;

        stream->state = STREAM_STATE_PAUSED;
        printf("UIO SND|INFO: Stream stopped\n");
        return SDDF_SND_S_OK;
    }
}

sddf_snd_status_code_t handle_command(stream_t *stream, sddf_snd_command_t *cmd, bool *blocked)
{
    switch (cmd->code) {
    case SDDF_SND_CMD_PCM_SET_PARAMS:
        return stream_set_params(stream, &cmd->set_params);
    case SDDF_SND_CMD_PCM_PREPARE:
        return stream_prepare(stream);
    case SDDF_SND_CMD_PCM_RELEASE:
        return stream_release(stream);
    case SDDF_SND_CMD_PCM_START:
        return stream_start(stream);
    case SDDF_SND_CMD_PCM_STOP:
        return stream_stop(stream, blocked);
    default:
        printf("UIO SND|ERR: unknown command %d\n", cmd->code);
        return SDDF_SND_S_BAD_MSG;
    }
}

static void flush_commands(stream_t *stream)
{
    while (!sddf_snd_ring_empty(&stream->commands.state)) {

        sddf_snd_command_t *cmd = sddf_snd_cmd_ring_front(&stream->commands);

        bool blocked = false;
        sddf_snd_status_code_t status = handle_command(stream, cmd, &blocked);

        if (blocked) {
            break;
        }
        sddf_snd_response_t response;
        response.cookie = cmd->cookie;
        response.status = status;

        if (sddf_snd_enqueue_response(&stream->responses, &response) != 0) {
            printf("UIO SND|ERR: Failed to enqueue response");
            break;
        }

        notify_main(stream);

        if (cmd->code == SDDF_SND_CMD_PCM_START) {
            printf("UIO SND|INFO: Updating prefill on start\n");
        }

        sddf_snd_ring_dequeue(&stream->commands.state);
    }
}

static void timer_tick(stream_t *stream)
{
    if (stream->state == STREAM_STATE_PLAYING) {
        int responses_sent = flush_playback(stream, INT_MAX);

        if (responses_sent == 0) {
            if (send_response(stream)) {
                notify_main(stream);
            } else {
                printf("UIO SND|WARN: No response to send\n");
            }
        }
    }
    flush_commands(stream);
}

static void *stream_entrypoint(void *data)
{
    stream_t *stream = data;
    struct pollfd fds[2];
    fds[COMMAND_FD].fd = stream->command_fd;
    fds[COMMAND_FD].events = POLLIN;
    fds[TIMER_FD].fd = stream->timer_fd;
    fds[TIMER_FD].events = POLLIN;

    uint64_t message;

    for (;;) {
        int nfds = stream->timer_enabled ? 2 : 1;

        int ready = poll(fds, nfds, -1);
        if (ready == -1) {
            perror("UIO SND|ERR: Failed to poll descriptors");
            return NULL;
        }

        if (fds[COMMAND_FD].revents & POLLIN) {
            if (read(stream->command_fd, &message, sizeof(message)) != sizeof(message)) {
                break;
            }
            flush_commands(stream);
        }
        if (stream->timer_enabled && fds[TIMER_FD].revents & POLLIN) {
            if (read(stream->timer_fd, &message, sizeof(message)) != sizeof(message)) {
                break;
            }
            timer_tick(stream);
        }
    }

    printf("UIO SND|ERR: Failed to read fd\n");
    return NULL;
}

stream_t *stream_open(sddf_snd_pcm_info_t *info, const char *device, snd_pcm_stream_t direction,
                      translation_state_t translate)
{
    stream_t *stream = malloc(sizeof(stream_t));
    if (stream == NULL) {
        printf("No enough memory\n");
        return NULL;
    }

    memset(info, 0, sizeof(sddf_snd_pcm_info_t));
    memset(stream, 0, sizeof(stream_t));

    int err;
    snd_pcm_t *handle = NULL;
    snd_pcm_hw_params_t *hw_params = NULL;
    snd_pcm_sw_params_t *sw_params = NULL;

    err = snd_pcm_open(&handle, device, direction, SND_PCM_NONBLOCK);
    if (err) {
        print_err(err, "Failed to open stream");
        goto fail;
    }

    err = snd_pcm_hw_params_malloc(&hw_params);
    if (err) {
        print_err(err, "Cannot allocate hardware parameter structure");
        goto fail;
    }

    err = snd_pcm_sw_params_malloc(&sw_params);
    if (err) {
        print_err(err, "Cannot allocate software parameter structure");
        goto fail;
    }

    err = snd_pcm_hw_params_any(handle, hw_params);
    if (err) {
        print_err(err, "Cannot initialize hardware parameter structure");
        goto fail;
    }

    unsigned int channels_min, channels_max;
    err = snd_pcm_hw_params_get_channels_min(hw_params, &channels_min);
    if (err) {
        print_err(err, "Cannot get min channels");
        goto fail;
    }

    err = snd_pcm_hw_params_get_channels_max(hw_params, &channels_max);
    if (err) {
        print_err(err, "Cannot get max channels");
        goto fail;
    }

    uint64_t rates = sddf_rates_from_hw_params(handle, hw_params);
    uint64_t formats = sddf_formats_from_hw_params(handle, hw_params);

    info->formats = formats;
    info->rates = rates;
    info->direction = direction == SND_PCM_STREAM_PLAYBACK ? SDDF_SND_D_OUTPUT : SDDF_SND_D_INPUT;
    info->channels_min = channels_min;
    info->channels_max = channels_max;

    stream->handle = handle;
    stream->state = STREAM_STATE_UNSET;
    stream->direction = direction;
    stream->hw_params = hw_params;
    stream->sw_params = sw_params;
    stream->translate = translate;

    stream->command_fd = eventfd(0, 0);
    stream->response_fd = eventfd(0, 0);
    stream->timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);

    sddf_snd_ring_init(&stream->commands.state,  SDDF_SND_NUM_BUFFERS);
    sddf_snd_ring_init(&stream->responses.state, SDDF_SND_NUM_BUFFERS);
    sddf_snd_ring_init(&stream->tx_used.state, SDDF_SND_NUM_BUFFERS);
    sddf_snd_ring_init(&stream->tx_free.state, SDDF_SND_NUM_BUFFERS);
    sddf_snd_ring_init(&stream->rx_free.state, SDDF_SND_NUM_BUFFERS);
    sddf_snd_ring_init(&stream->rx_used.state, SDDF_SND_NUM_BUFFERS);

    stream->played_frames = pcm_queue_create(PCM_QUEUE_SIZE);

    err = pthread_create(&stream->thread, NULL, stream_entrypoint, stream);
    if (err) {
        printf("UIO SND|ERR: failed to create stream thread\n");
        goto fail;
    }

    return stream;

fail:
    if (hw_params)
        snd_pcm_hw_params_free(hw_params);
    if (sw_params)
        snd_pcm_sw_params_free(sw_params);
    if (handle)
        snd_pcm_close(handle);
    if (stream)
        free(stream);
    return NULL;
}

int stream_enqueue_command(stream_t *stream, sddf_snd_command_t *cmd)
{
    return sddf_snd_enqueue_cmd(&stream->commands, cmd);
}

int stream_dequeue_response(stream_t *stream, sddf_snd_response_t *res)
{
    return sddf_snd_dequeue_response(&stream->responses, res);
}

int stream_enqueue_tx_used(stream_t *stream, sddf_snd_pcm_data_t *pcm)
{
    return sddf_snd_enqueue_pcm_data(&stream->tx_used, pcm);
}

int stream_dequeue_tx_free(stream_t *stream, sddf_snd_pcm_data_t *pcm)
{
    return sddf_snd_dequeue_pcm_data(&stream->tx_free, pcm);
}

int stream_enqueue_rx_free(stream_t *stream, sddf_snd_pcm_data_t *pcm)
{
    return sddf_snd_enqueue_pcm_data(&stream->rx_free, pcm);
}

int stream_dequeue_rx_used(stream_t *stream, sddf_snd_pcm_data_t *pcm)
{
    return sddf_snd_dequeue_pcm_data(&stream->rx_used, pcm);
}

int stream_notify(stream_t *stream)
{
    uint64_t message = 1;
    return write(stream->command_fd, &message, sizeof(message));
}

int stream_response_fd(stream_t *stream)
{
    return stream->response_fd;
}

static const char *stream_state_str(stream_state_t state)
{
    switch (state) {
    case STREAM_STATE_UNSET:
        return "unset";
    case STREAM_STATE_SET:
        return "set";
    case STREAM_STATE_PLAYING:
        return "prepared";
    case STREAM_STATE_PAUSED:
        return "paused";
    case STREAM_STATE_IO_ERR:
        return "IO ERROR";
    default:
        return "unknown";
    }
}

static char *snd_state_str(snd_pcm_t *handle)
{
    switch (snd_pcm_state(handle)) {
    case SND_PCM_STATE_OPEN:
        return "SND_PCM_STATE_OPEN";
        break;
    case SND_PCM_STATE_SETUP:
        return "SND_PCM_STATE_SETUP";
        break;
    case SND_PCM_STATE_PREPARED:
        return "SND_PCM_STATE_PREPARED";
        break;
    case SND_PCM_STATE_RUNNING:
        return "SND_PCM_STATE_RUNNING";
        break;
    case SND_PCM_STATE_XRUN:
        return "SND_PCM_STATE_XRUN";
        break;
    case SND_PCM_STATE_PAUSED:
        return "SND_PCM_STATE_PAUSED";
        break;
    case SND_PCM_STATE_SUSPENDED:
        return "SND_PCM_STATE_SUSPENDED";
        break;
    case SND_PCM_STATE_DISCONNECTED:
        return "SND_PCM_STATE_DISCONNECTED";
        break;
    default:
        return "unknown";
    }
}
