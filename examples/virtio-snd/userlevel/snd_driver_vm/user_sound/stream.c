#include "stream.h"
#include "convert.h"
#include "queue.h"
#include <alsa/pcm.h>
#include <limits.h>
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
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;

    snd_pcm_stream_t direction;
    stream_state_t state;
    translation_state_t translate;

    cmd_queue_t *commands;
    cmd_queue_t *staged_pcms;

    int frame_size;
    snd_pcm_sframes_t buffer_size;
    snd_pcm_sframes_t period_size;
    snd_pcm_sframes_t start_threshold;

    snd_pcm_sframes_t prefilled;

    snd_pcm_sframes_t pcm_offset;

    struct pollfd *fds;
    int nfds;

    respond_fn respond;
    tx_free_fn tx_free;
    void *user_data;
};

struct alsa_params {
    unsigned resample;
    snd_pcm_format_t format;
    unsigned channels;
    unsigned rate;
    unsigned latency_us;
    unsigned period_us; // time between hardware interrupts
    // unsigned period_bytes;
};

struct buffer_state {
    snd_pcm_sframes_t buffer_size;
    snd_pcm_sframes_t period_size;
};

typedef enum cmd_state {
    CMD_STATE_COMPLETE = 0,
    CMD_STATE_BLOCK,
    CMD_STATE_STAGE,
} cmd_state_t;

typedef struct cmd_result {
    sddf_snd_status_code_t status;
    cmd_state_t state;
} cmd_result_t;

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

stream_t *stream_open(sddf_snd_pcm_info_t *info, const char *device, snd_pcm_stream_t direction,
                      translation_state_t translate, respond_fn respond, tx_free_fn tx_free,
                      void *user_data)
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
    struct pollfd *fds = NULL;
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

    // int dir;
    // snd_pcm_uframes_t period_size;
    // err = snd_pcm_hw_params_get_period_size(hw_params, &period_size, &dir);
    // if (err < 0) {
    //     printf("Unable to get period size for playback: %s\n", snd_strerror(err));
    //     goto fail;
    // }

    uint64_t rates = sddf_rates_from_hw_params(handle, hw_params);
    uint64_t formats = sddf_formats_from_hw_params(handle, hw_params);

    int nfds = snd_pcm_poll_descriptors_count(handle);
    if (nfds <= 0) {
        printf("UIO SND|ERR: Invalid poll descriptors count\n");
        err = -1;
        goto fail;
    }

    fds = malloc(sizeof(struct pollfd) * nfds);
    if (fds == NULL) {
        printf("UIO SND|ERR: No enough memory\n");
        goto fail;
    }
    if ((err = snd_pcm_poll_descriptors(handle, fds, nfds)) < 0) {
        printf("UIO SND|ERR: Unable to obtain poll descriptors for playback: %s\n",
               snd_strerror(err));
        goto fail;
    }

    for (int i = 0; i < nfds; i++) {
        printf("init fd %d: ", fds[i].fd);
        if (fds[i].events & POLLIN)  printf("in ");
        if (fds[i].events & POLLOUT) printf("out ");
        if (fds[i].events & POLLPRI) printf("pri ");
        if (fds[i].events & POLLERR) printf("err ");
    }

    info->formats = formats;
    info->rates = rates;
    info->direction = direction == SND_PCM_STREAM_PLAYBACK ? SDDF_SND_D_OUTPUT : SDDF_SND_D_INPUT;
    info->channels_min = channels_min;
    info->channels_max = channels_max;

    stream->handle = handle;
    stream->fds = fds;
    stream->nfds = nfds;
    stream->state = STREAM_STATE_UNSET;
    stream->direction = direction;
    stream->hw_params = hw_params;
    stream->sw_params = sw_params;
    stream->translate = translate;
    stream->respond = respond;
    stream->tx_free = tx_free;
    stream->user_data = user_data;

    if (direction == SND_PCM_STREAM_PLAYBACK) {
        stream->commands = cmd_queue_create(PLAYBACK_QUEUE_SIZE);
        stream->staged_pcms = cmd_queue_create(STAGING_QUEUE_SIZE);

        if (stream->staged_pcms == NULL) {
            printf("UIO SND|ERR: Failed to create staged_pcms queue\n");
            goto fail;
        }
    } else {
        stream->commands = cmd_queue_create(RECORDING_QUEUE_SIZE);
        stream->staged_pcms = NULL;
    }

    if (stream->commands == NULL) {
        printf("UIO SND|ERR: Failed to create command queue\n");
        goto fail;
    }

    return stream;

fail:
    if (fds)
        free(fds);
    if (hw_params)
        snd_pcm_hw_params_free(hw_params);
    if (sw_params)
        snd_pcm_sw_params_free(sw_params);
    if (handle)
        snd_pcm_close(handle);
    if (stream->commands)
        free(stream->commands);
    if (stream->staged_pcms)
        free(stream->staged_pcms);
    if (stream)
        free(stream);
    return NULL;
}

void stream_close(stream_t *stream)
{
    int err = snd_pcm_close(stream->handle);
    if (err < 0) {
        printf("Failed to close stream: %s\n", snd_strerror(err));
    }

    snd_pcm_hw_params_free(stream->hw_params);
    snd_pcm_sw_params_free(stream->sw_params);

    free(stream);
}

void stream_enqueue_command(stream_t *stream, sddf_snd_command_t *cmd)
{
    cmd_queue_enqueue(stream->commands, cmd);
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
        return code;
    }

    snd_pcm_sframes_t start_threshold;
    code = set_swparams(handle, stream->sw_params, &buffer_state, &start_threshold);

    if (code != SDDF_SND_S_OK) {
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
        return SDDF_SND_S_BAD_MSG;
    }

    int err = snd_pcm_prepare(stream->handle);
    if (err) {
        printf("UIO SND|ERR: Failed to prepare stream: %s\n", snd_strerror(err));
        return SDDF_SND_S_IO_ERR;
    }

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

    // Change state even if drop fails
    stream->state = STREAM_STATE_SET;

    if (stream->staged_pcms) {
        cmd_queue_clear(stream->staged_pcms);
    }

    printf("UIO SND|INFO: Dropping frames\n");
    int err = snd_pcm_drop(stream->handle);
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
    // @alexbr: figure out how to disable autostart
    if (snd_pcm_state(stream->handle) != SND_PCM_STATE_RUNNING) {
        int err = snd_pcm_start(stream->handle);
        if (err) {
            printf("UIO SND|ERR: failed to start stream: %s\n", snd_strerror(err));
            return SDDF_SND_S_IO_ERR;
        }
    }
    stream->state = STREAM_STATE_PLAYING;
    return SDDF_SND_S_OK;
}

static cmd_result_t stream_stop(stream_t *stream)
{
    cmd_result_t result;
    result.state = CMD_STATE_COMPLETE;

    if (stream->state != STREAM_STATE_PLAYING && stream->state != STREAM_STATE_DRAINING) {
        printf("UIO SND|ERR: Cannot stop stream now, invalid state\n");

        result.status = SDDF_SND_S_BAD_MSG;
        return result;
    }

    int err = snd_pcm_drain(stream->handle);

    if (err == -EAGAIN) {
        // printf("UIO SND|INFO: Drain blocked\n");

        stream->state = STREAM_STATE_DRAINING;
        result.state = CMD_STATE_BLOCK;

    } else if (err < 0) {
        printf("UIO SND|ERR: Failed to drain stream: %s\n", snd_strerror(err));

        result.status = SDDF_SND_S_IO_ERR;
    } else {
        printf("UIO SND|INFO: Drain finished\n");

        stream->state = STREAM_STATE_PAUSED;
        result.status = SDDF_SND_S_OK;
    }

    return result;
}

static snd_pcm_sframes_t stream_prefill_remaining(stream_t *stream)
{
    return stream->start_threshold - PREFILL_GAP - stream->prefilled;
}

static bool stream_prefilled(stream_t *stream)
{
    snd_pcm_sframes_t remaining = stream_prefill_remaining(stream);
    assert(remaining >= 0);
    return remaining == 0;
}

static bool stream_accepting_pcm(stream_t *stream)
{
    return (stream->state == STREAM_STATE_PAUSED && !stream_prefilled(stream))
        || stream->state == STREAM_STATE_PLAYING;
}

static cmd_result_t stream_tx(stream_t *stream, sddf_snd_pcm_data_t *pcm)
{
    cmd_result_t result;

    // if (stream->state == STREAM_STATE_PAUSED) {
    //     printf("UIO SND|INFO: Prefilled %ld/%ld (%s)\n",
    //         stream->prefilled, stream->start_threshold,
    //         stream_prefilled(stream) ? "yes" : "no");
    // }

    if (!stream_accepting_pcm(stream)) {
        if (stream->state == STREAM_STATE_PAUSED && stream_prefilled(stream)) {
            result.state = CMD_STATE_STAGE;
        } else {
            result.state = CMD_STATE_BLOCK;
        }
        result.status = SDDF_SND_S_OK;
        return result;
    }

    // printf("UIO SND|INFO: flushing\n");
    // stream_debug_state(stream);

    const void *pcm_data = translate_tx(&stream->translate, (void *)pcm->addr);

    const snd_pcm_sframes_t frames_remaining = pcm->len / stream->frame_size - stream->pcm_offset;

    snd_pcm_sframes_t to_write = frames_remaining;
    if (stream->state == STREAM_STATE_PAUSED) {
        to_write = min(stream_prefill_remaining(stream), to_write);
    }

    while (to_write > 0) {

        snd_pcm_sframes_t written = snd_pcm_writei(
            stream->handle, pcm_data + stream->pcm_offset * stream->frame_size, to_write);

        if (written == -EAGAIN) {
            result.state = CMD_STATE_BLOCK;
            result.status = SDDF_SND_S_OK;
            return result;

        } else if (written < 0) {
            printf("UIO SND|ERR: failed to flush pcm: %s, state %s\n", snd_strerror(written),
                   snd_state_str(stream->handle));

            // @alexbr: should we have an error state?
            stream->state = STREAM_STATE_PAUSED;

            result.state = CMD_STATE_BLOCK;
            result.status = SDDF_SND_S_IO_ERR;
            return result;
        }

        // int consec = 0;
        // int max_consec = 0;
        // int16_t consec_char = 999;
        // int max_diff = 0;
        // int max_diff_idx = -1;

        // int16_t prev = ((int16_t *)pcm_data + stream->pcm_offset)[0];
        // const int16_t first = prev;
        // for (int i = 1; i < written; i++) {
        //     int16_t sample = ((int16_t *)pcm_data + stream->pcm_offset)[i];
        //     if (sample == prev) {
        //         consec++;
        //         if (consec > max_consec) {
        //             max_consec = consec;
        //             consec_char = sample;
        //         }
        //     }
        //     int diff = abs(sample - prev);
        //     if (diff > max_diff) {
        //         max_diff = diff;
        //         max_diff_idx = i;
        //     }
        //     prev = sample;
        // }
        // printf("Written %ld, max consec %d (sample %d), max diff %d. first/last %6d %6d\n",
        //     written, max_consec, consec_char, max_diff, first, prev);

        // for (int i = 0; i < 20; i++) {
        //     int16_t sample = ((int16_t *)pcm_data + stream->pcm_offset)[i];
        //     printf("%6d ", sample);
        //     if (i % 32 == 31) putchar('\n');
        // }
        
        // if (max_diff > 3000) {
        //     int left = max_diff_idx - 12 >= 0 ? max_diff_idx - 12 : 0;
        //     int right = max_diff_idx + 12 < written ? max_diff_idx + 12 : written-1;
        //     printf("%d -> [%d, %d)\n", max_diff_idx, left, right);
        //     for (int i = left; i < right; i++) {
        //         int16_t sample = ((int16_t *)pcm_data + stream->pcm_offset)[i];
        //         printf("%6d ", sample);
        //     }
        // }

        if (stream->state == STREAM_STATE_PAUSED) {
            stream->prefilled += written;
            printf("UIO SND|INFO: Prefilled %ld/%ld\n", stream->prefilled, stream->start_threshold);
        }

        stream->pcm_offset += written;
        to_write -= written;
    }

    stream->pcm_offset = 0;

    // printf("\nUIO SND|INFO: END TX\n");

    result.state = CMD_STATE_COMPLETE;
    result.status = SDDF_SND_S_OK;
    return result;
}

cmd_result_t handle_command(stream_t *stream, sddf_snd_command_t *cmd)
{
    cmd_result_t result;
    result.state = CMD_STATE_COMPLETE;

    switch (cmd->code) {
    case SDDF_SND_CMD_PCM_SET_PARAMS: {
        result.status = stream_set_params(stream, &cmd->set_params);
        break;
    }
    case SDDF_SND_CMD_PCM_PREPARE:
        result.status = stream_prepare(stream);
        break;
    case SDDF_SND_CMD_PCM_RELEASE:
        result.status = stream_release(stream);
        break;
    case SDDF_SND_CMD_PCM_START:
        result.status = stream_start(stream);
        break;
    case SDDF_SND_CMD_PCM_STOP:
        result = stream_stop(stream);
        break;
    case SDDF_SND_CMD_PCM_TX:
        result = stream_tx(stream, &cmd->pcm);
        break;
    default:
        printf("UIO SND|ERR: unknown command %d\n", cmd->code);
        result.status = SDDF_SND_S_BAD_MSG;
    }

    return result;
}

static void update_staged(stream_t *stream)
{
    sddf_snd_command_t *cmd;
    while ((cmd = cmd_queue_front(stream->staged_pcms)) != NULL) {
        assert(cmd->code == SDDF_SND_CMD_PCM_TX);

        cmd_result_t result = stream_tx(stream, &cmd->pcm);
        switch (result.state) {
        case CMD_STATE_COMPLETE:
            // printf("UIO SND|INFO: responding to staged tx with %s, cookie %u\n",
            //        sddf_snd_status_code_str(result.status), cmd->cookie);

            stream->tx_free(cmd->pcm.addr, SDDF_SND_PCM_BUFFER_SIZE, stream->user_data);
            stream->respond(cmd->cookie, result.status, stream->buffer_size * stream->frame_size,
                            stream->user_data);
            break;
        case CMD_STATE_BLOCK:
        case CMD_STATE_STAGE:
            // printf("UIO SND|INFO: staged blocked\n");
            return;
        }

        bool did_dequeue = cmd_queue_dequeue(stream->staged_pcms);
        assert(did_dequeue);
    }
}

void stream_update(stream_t *stream)
{
    if (stream->direction == SND_PCM_STREAM_PLAYBACK) {
        update_staged(stream);
    }

    sddf_snd_command_t *cmd;
    while ((cmd = cmd_queue_front(stream->commands)) != NULL) {

        cmd_result_t result = handle_command(stream, cmd);

        switch (result.state) {
        case CMD_STATE_COMPLETE:

            // printf("UIO SND|INFO: %s responding to %s with %s, cookie %u\n",
            //        stream->direction == SND_PCM_STREAM_PLAYBACK ? "playback" : "capture",
            //        sddf_snd_command_code_str(cmd->code), sddf_snd_status_code_str(result.status),
            //        cmd->cookie);
            if (cmd->code == SDDF_SND_CMD_PCM_TX) {
                stream->tx_free(cmd->pcm.addr, SDDF_SND_PCM_BUFFER_SIZE, stream->user_data);
            }

            stream->respond(cmd->cookie, result.status, stream->buffer_size * stream->frame_size,
                            stream->user_data);
            break;
        case CMD_STATE_BLOCK:
            // printf("UIO SND|INFO: blocked\n");
            return;
        case CMD_STATE_STAGE:
            printf("UIO SND|INFO: staging tx with cookie %u\n", cmd->cookie);
            cmd_queue_enqueue(stream->staged_pcms, cmd);
            break;
        }

        bool did_dequeue = cmd_queue_dequeue(stream->commands);
        assert(did_dequeue);
    }
}

snd_pcm_t *stream_handle(stream_t *stream)
{
    return stream->handle;
}

bool stream_should_poll(stream_t *stream)
{
    return !cmd_queue_empty(stream->commands);
}

struct pollfd *stream_fds(stream_t *stream)
{
    return stream->fds;
}

int stream_nfds(stream_t *stream)
{
    return stream->nfds;
}

int stream_demangle_fds(stream_t *stream, struct pollfd *pfds, unsigned int nfds,
                        unsigned short *revents)
{
    return snd_pcm_poll_descriptors_revents(stream->handle, pfds, nfds, revents);
}

snd_pcm_stream_t stream_direction(stream_t *stream)
{
    return stream->direction;
}

void stream_debug_state(stream_t *stream)
{
    printf("UIO SND|INFO: staged %d, commands %d, state %s, snd_state %s\n",
           stream->staged_pcms ? cmd_queue_size(stream->staged_pcms) : 0,
           cmd_queue_size(stream->commands), stream_state_str(stream->state),
           snd_state_str(stream->handle));
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
    case STREAM_STATE_DRAINING:
        return "draining";
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
    case SND_PCM_STATE_DRAINING:
        return "SND_PCM_STATE_DRAINING";
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
