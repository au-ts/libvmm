#include "stream.h"
#include "convert.h"
#include "queue.h"
#include <alsa/pcm.h>
#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// This queue may have many TX buffers along side commands.
#define PLAYBACK_QUEUE_SIZE (SDDF_SND_NUM_BUFFERS / 4)
#define STAGING_QUEUE_SIZE 8
// This queue will only ever have START, STOP, etc.
#define RECORDING_QUEUE_SIZE 8

// 0.5 seconds
#define LATENCY 500000
// Time between hardware interrupts
#define PERIOD_TIME 100000

// Number of frames before start_threshold we prefill to.
// Ensures we don't autoplay.
#define PREFILL_GAP 16

#define BITS_PER_BYTE 8

struct stream {
    pthread_t thread;
    int send_cmd;
    int recv_cmd;
    int send_res;
    int recv_res;

    int pcm_fd;

    snd_pcm_t *handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;

    snd_pcm_stream_t direction;
    stream_state_t state;
    translation_state_t translate;

    int frame_size;
    snd_pcm_sframes_t buffer_size;
    snd_pcm_sframes_t period_size;
    snd_pcm_sframes_t start_threshold;

    snd_pcm_sframes_t prefilled;
    snd_pcm_sframes_t before_start;
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

    stream->pcm_fd = open("out.pcm", O_WRONLY | O_CREAT);
    if (stream->pcm_fd < 0) {
        perror("UIO SND|ERR: Failed to open out.pcm");
        return SDDF_SND_S_IO_ERR;
    }

    int err = snd_pcm_prepare(stream->handle);
    if (err) {
        printf("UIO SND|ERR: Failed to prepare stream: %s\n", snd_strerror(err));
        return SDDF_SND_S_IO_ERR;
    }

    printf("UIO SND|INFO: Prepared stream\n");

    stream->state = STREAM_STATE_PAUSED;
    stream->prefilled = 0;

    return SDDF_SND_S_OK;
}

static sddf_snd_status_code_t stream_release(stream_t *stream)
{
    if (stream->state != STREAM_STATE_PAUSED) {
        printf("UIO SND|ERR: Cannot release stream now, not paused\n");
        return SDDF_SND_S_BAD_MSG;
    }

    int err = close(stream->pcm_fd);
    assert(!err);
    stream->pcm_fd = -1;

    // Change state even if drop fails
    stream->state = STREAM_STATE_SET;
    stream->prefilled = 0;

    printf("UIO SND|INFO: Dropping frames\n");
    err = snd_pcm_drop(stream->handle);
    if (err) {
        printf("UIO SND|ERR: Failed to release stream: %s\n", snd_strerror(err));
        return SDDF_SND_S_IO_ERR;
    }

    return SDDF_SND_S_OK;
}

static sddf_snd_status_code_t stream_start(stream_t *stream)
{
    if (stream->state != STREAM_STATE_PAUSED) {
        printf("UIO SND|ERR: Cannot start stream now, invalid state\n");
        return SDDF_SND_S_BAD_MSG;
    }

    int err = snd_pcm_start(stream->handle);
    if (err) {
        printf("UIO SND|ERR: failed to start stream: %s\n", snd_strerror(err));
        return SDDF_SND_S_IO_ERR;
    }
    printf("UIO SND|INFO: Started stream\n");

    stream->state = STREAM_STATE_PLAYING;
    return SDDF_SND_S_OK;
}

static sddf_snd_status_code_t stream_stop(stream_t *stream)
{
    if (stream->state != STREAM_STATE_PLAYING) {
        printf("UIO SND|ERR: Cannot stop stream now, invalid state\n");

        return SDDF_SND_S_BAD_MSG;
    }

    printf("UIO SND|INFO: Draining...\n");
    int err = snd_pcm_drain(stream->handle);

    stream->state = STREAM_STATE_PAUSED;

    if (err < 0) {
        printf("UIO SND|ERR: Failed to drain stream: %s\n", snd_strerror(err));
        return SDDF_SND_S_IO_ERR;
    }

    printf("UIO SND|INFO: Drain finished\n");
    return SDDF_SND_S_OK;
}

static snd_pcm_sframes_t stream_prefill_remaining(stream_t *stream)
{
    return stream->start_threshold - PREFILL_GAP - stream->prefilled;
}

static bool stream_can_prefill(stream_t *stream, snd_pcm_sframes_t amount)
{
    snd_pcm_sframes_t remaining = stream_prefill_remaining(stream);
    assert(remaining >= 0);
    return remaining >= amount;
}

// static bool stream_accepting_pcm(stream_t *stream, snd_pcm_sframes_t amount)
// {
//     return (stream->state == STREAM_STATE_PAUSED && stream_can_prefill(stream, amount)) ||
//         (stream->state == STREAM_STATE_PLAYING) && (stream->prefilled

//     // return (stream->state == STREAM_STATE_PAUSED && cmd_queue_empty(stream->staged_pcms)
//     //         && stream_can_prefill(stream, amount))
//     //     || stream->state == STREAM_STATE_PLAYING;
//     return stream->state == STREAM_STATE_PLAYING;
// }

static void print_bytes(const void *data)
{
    for (int i = 0; i < 16; i++) {
        printf("%02x ", ((uint8_t *)data)[i]);
    }
    printf("\n");
}

static sddf_snd_status_code_t stream_tx(stream_t *stream, sddf_snd_pcm_data_t *pcm, uint32_t id)
{
    snd_pcm_sframes_t start_offset = 0;
    snd_pcm_sframes_t end_offset = pcm->len / stream->frame_size;

    switch (stream->state) {
    case STREAM_STATE_PAUSED: {
        snd_pcm_sframes_t remaining = stream_prefill_remaining(stream);
        end_offset = min(end_offset, remaining);

        stream->prefilled += end_offset;
        break;
    }
    case STREAM_STATE_PLAYING:

        start_offset = min(stream->prefilled, end_offset);
        stream->prefilled -= start_offset;

        // if (stream->prefilled >= to_write) {
        //     printf("Already played (id %d)\n", id);
        //     // We have already played this frame, skip.
        //     stream->prefilled -= to_write;
        //     return SDDF_SND_S_OK;
        // }
        // printf("Playing (id %d)\n", id);
        // Otherwise, play frame
        break;
    default:
        return SDDF_SND_S_BAD_MSG;
    }

    snd_pcm_sframes_t to_write = end_offset - start_offset;

    const void *pcm_data
        = translate_tx(&stream->translate, (void *)pcm->addr) + (start_offset * stream->frame_size);
    
    if (stream->state == STREAM_STATE_PAUSED) {
        printf("UIO SND|INFO: Prefill: ");
        if (end_offset == pcm->len / stream->frame_size) {
            printf("whole %ld frames (id %d)\n", end_offset, id);
        } else if (end_offset > 0) {
            printf("partial %ld/%u frames (id %d)\n", end_offset,
                   pcm->len / stream->frame_size, id);
        } else {
            printf("none (id %d)\n", id);
        }
    } else if (stream->state == STREAM_STATE_PLAYING) {
        printf("UIO SND|INFO: Play: ");
        if (start_offset == end_offset) {
            printf("skip (prefill %ld -> %ld) (id %d)\n", stream->prefilled + start_offset, stream->prefilled, id);
        } else if (start_offset > 0) {
            printf("partial frames [%ld, %ld) (id %d)\n", start_offset, end_offset, id);
        } else {
            printf("full frames [%ld, %ld) (id %d)\n", start_offset, end_offset, id);
        }
    }

    while (to_write > 0) {

#if 0
        snd_pcm_sframes_t written = snd_pcm_writei(stream->handle, pcm_data, to_write);

        if (written < 0) {
            printf("UIO SND|ERR: failed to flush pcm: %s, state %s\n", snd_strerror(written),
                   snd_state_str(stream->handle));

            // @alexbr: should we have an error state?
            stream->state = STREAM_STATE_PAUSED;
            return SDDF_SND_S_IO_ERR;
        }

        if (written < to_write) {
            printf("UIO SND|INFO: Warning: tried to write %ld but wrote %ld\n", to_write, written);
        }

        if (stream->state == STREAM_STATE_PAUSED) {
            printf("UIO SND|INFO: Prefilled %ld/%ld (id %u)\n", stream->prefilled,
                   stream->start_threshold, id);
        } else {
            printf("UIO SND|INFO: Wrote %ld (id %u)\n", written, id);
        }
#endif

        print_bytes(pcm_data);

        int fwritten = write(stream->pcm_fd, pcm_data, to_write * stream->frame_size);
        if (fwritten != to_write * stream->frame_size) {
            printf("Bad write\n");
            exit(1);
        }

        to_write -= to_write;

        // to_write -= written;
        // pcm_data += written * stream->frame_size;
    }

    return SDDF_SND_S_OK;
}

sddf_snd_status_code_t handle_command(stream_t *stream, sddf_snd_command_t *cmd)
{
    switch (cmd->code) {
    case SDDF_SND_CMD_PCM_SET_PARAMS: {
        return stream_set_params(stream, &cmd->set_params);
    }
    case SDDF_SND_CMD_PCM_PREPARE:
        printf("UIO SND|INFO: handle prepare\n");
        return stream_prepare(stream);
    case SDDF_SND_CMD_PCM_RELEASE:
        printf("UIO SND|INFO: handle release\n");
        return stream_release(stream);
    case SDDF_SND_CMD_PCM_START:
        printf("UIO SND|INFO: handle start\n");
        return stream_start(stream);
    case SDDF_SND_CMD_PCM_STOP:
        printf("UIO SND|INFO: handle stop\n");
        return stream_stop(stream);
    case SDDF_SND_CMD_PCM_TX:
        return stream_tx(stream, &cmd->pcm, cmd->id);
    default:
        printf("UIO SND|ERR: unknown command %d\n", cmd->code);
        return SDDF_SND_S_BAD_MSG;
    }
}

static void send_response(stream_t *stream, stream_response_t *response)
{
    int nwritten = write(stream->send_res, response, sizeof(*response));
    if (nwritten != sizeof(*response)) {
        printf("UIO SND|ERR: failed to enqueue response\n");
    }
}

static void *stream_entrypoint(void *data)
{
    stream_t *stream = data;

    sddf_snd_command_t cmd;
    int nbytes;
    while ((nbytes = read(stream->recv_cmd, &cmd, sizeof(cmd))) == sizeof(cmd)) {

        sddf_snd_status_code_t status = handle_command(stream, &cmd);
        stream_response_t msg;
        msg.response.cookie = cmd.cookie;
        msg.response.status = status;
        msg.response.latency_bytes = stream->buffer_size * stream->frame_size;

        if (cmd.code == SDDF_SND_CMD_PCM_TX) {
            msg.has_tx_free = true;
            msg.tx_free.addr = cmd.pcm.addr;
            msg.tx_free.len = SDDF_SND_PCM_BUFFER_SIZE;
        }

        send_response(stream, &msg);
    }

    printf("UIO SND: ERR: Stream recv %d bytes, exiting", nbytes);
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

    err = snd_pcm_open(&handle, device, direction, 0);
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

    int pipes[2];
    err = pipe(pipes);
    if (err) {
        printf("UIO SND|ERR: failed to create pipes\n");
        goto fail;
    }
    stream->send_cmd = pipes[1];
    stream->recv_cmd = pipes[0];

    err = pipe(pipes);
    if (err) {
        printf("UIO SND|ERR: failed to create pipes\n");
        goto fail;
    }
    stream->send_res = pipes[1];
    stream->recv_res = pipes[0];

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

void stream_enqueue_command(stream_t *stream, sddf_snd_command_t *cmd)
{
    int nwritten = write(stream->send_cmd, cmd, sizeof(*cmd));
    if (nwritten != sizeof(*cmd)) {
        printf("UIO SND|ERR: failed to enqueue command");
    }
}

int stream_res_fd(stream_t *stream)
{
    return stream->recv_res;
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
