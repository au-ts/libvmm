#include "stream.h"
#include "convert.h"
#include <alsa/pcm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define AVAIL_FRAMES 4
#define MIN_FRAMES 2

#define BITS_PER_BYTE 8

// 0.5 seconds
#define LATENCY 500000
// Time between hardware interrupts
#define PERIOD_TIME 100000

// Number of frames before start_threshold we prefill to.
// Ensures we don't autoplay.
#define PREFILL_GAP 16

struct stream {
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;

    snd_pcm_stream_t direction;
    stream_state_t state;
    sddf_snd_ring_state_t *consume_ring;

    int frame_size;
    snd_pcm_sframes_t buffer_size;
    snd_pcm_sframes_t period_size;
    snd_pcm_sframes_t start_threshold;

    snd_pcm_sframes_t prefilled;

    bool has_pcm;
    sddf_snd_pcm_data_t pcm_copy;
    void *pcm_data;
    snd_pcm_sframes_t pcm_offset;

    struct pollfd *fds;
    int nfds;
};

static void print_err(int err, const char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, snd_strerror(err));
}

static inline snd_pcm_sframes_t min(snd_pcm_sframes_t a, snd_pcm_sframes_t b)
{
    return a <= b ? a : b;
}

stream_t *stream_open(sddf_snd_pcm_info_t *info, const char *device, snd_pcm_stream_t direction,
                      sddf_snd_ring_state_t *consume_ring)
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
        printf("Invalid poll descriptors count\n");
        err = -1;
        goto fail;
    }

    fds = malloc(sizeof(struct pollfd) * nfds);
    if (fds == NULL) {
        printf("No enough memory\n");
        err = -ENOMEM;
        goto fail;
    }
    if ((err = snd_pcm_poll_descriptors(handle, fds, nfds)) < 0) {
        printf("Unable to obtain poll descriptors for playback: %s\n", snd_strerror(err));
        goto fail;
    }

    info->formats = formats;
    info->rates = rates;
    info->channels_min = channels_min;
    info->channels_max = channels_max;

    stream->handle = handle;
    stream->fds = fds;
    stream->nfds = nfds;
    stream->state = STREAM_STATE_UNSET;
    stream->direction = direction;
    stream->has_pcm = false;
    stream->consume_ring = consume_ring;
    // state->period_size = period_size;
    stream->hw_params = hw_params;
    stream->sw_params = sw_params;

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

void play(snd_pcm_t *handle);

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
    err = snd_pcm_hw_params_get_period_size(hw_params, &size, &dir);
    if (err < 0) {
        printf("Unable to get period size for playback: %s\n", snd_strerror(err));
        return SDDF_SND_S_IO_ERR;
    }
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

sddf_snd_status_code_t stream_set_params(stream_t *stream, sddf_snd_pcm_set_params_t *params)
{
    printf("UIO SND|INFO: set params stream %p, format %s, rate %u, channels %d, period_bytes %u\n",
           stream, sddf_snd_pcm_fmt_str(params->format), sddf_rate_to_alsa(params->rate),
           params->channels, params->period_bytes);

    bool state_valid = stream->state == STREAM_STATE_UNSET || stream->state == STREAM_STATE_SET;
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
    alsa_params.rate = rate;
    alsa_params.resample = 1;

    struct buffer_state buffer_state;
    sddf_snd_status_code_t code;

    code = set_hwparams(
        handle, stream->hw_params, SND_PCM_ACCESS_RW_INTERLEAVED, &alsa_params, &buffer_state);
    
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

    return SDDF_SND_S_OK;
}

sddf_snd_status_code_t stream_prepare(stream_t *stream)
{
    if (stream->state != STREAM_STATE_SET) {
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

sddf_snd_status_code_t stream_release(stream_t *stream)
{
    if (stream->state != STREAM_STATE_PAUSED) {
        printf("UIO SND|ERR: Cannot release stream now, not paused\n");
        return SDDF_SND_S_BAD_MSG;
    }

    // Change state even if drop fails
    stream->state = STREAM_STATE_SET;

    int err = snd_pcm_drop(stream->handle);
    if (err) {
        printf("UIO SND|ERR: Failed to release stream: %s\n", snd_strerror(err));
        return SDDF_SND_S_IO_ERR;
    }

    return SDDF_SND_S_OK;
}

sddf_snd_status_code_t stream_start(stream_t *stream)
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

sddf_snd_status_code_t stream_stop(stream_t *stream)
{
    if (stream->state != STREAM_STATE_PLAYING) {
        printf("UIO SND|ERR: Cannot stop stream now, invalid state\n");
        return SDDF_SND_S_BAD_MSG;
    }

    // Change state even if drain fails
    int err;
    while ((err = snd_pcm_drain(stream->handle)) == -EAGAIN)
        ;

    if (err) {
        printf("UIO SND|ERR: Failed to stop stream: %s\n", snd_strerror(err));
        return SDDF_SND_S_IO_ERR;
    }
    printf("UIO SND|INFO: STOPPED\n");

    stream->state = STREAM_STATE_PAUSED;

    return SDDF_SND_S_OK;
}

void stream_update(stream_t *stream)
{
    // if (stream->state == STREAM_STATE_DRAINING) {
    //     stream_debug_state(stream);

    //     int err = snd_pcm_drain(stream->handle);
    //     if (err && err != -EAGAIN) {
    //         printf("UIO SND|ERR: Failed to drain stream: %s\n", snd_strerror(err));
    //     }

    //     if (snd_pcm_state(stream->handle) != SND_PCM_STATE_DRAINING) {
    //         stream->state = STREAM_STATE_PAUSED;
    //     }
    // }
}

snd_pcm_t *stream_handle(stream_t *stream)
{
    return stream->handle;
}

bool stream_should_poll(stream_t *stream)
{
    // @alexbr: we could also wait for produce buffer to not be full
    return stream->has_pcm && !sddf_snd_ring_empty(stream->consume_ring);
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

snd_pcm_stream_t stream_direction(stream_t *stream)
{
    return stream->direction;
}

void stream_set_pcm(stream_t *stream, sddf_snd_pcm_data_t *pcm, void *pcm_data)
{
    assert(!stream->has_pcm);
    stream->pcm_copy = *pcm;
    stream->pcm_data = pcm_data;
    stream->pcm_offset = 0;
    stream->has_pcm = true;
}

bool stream_has_pcm(stream_t *stream)
{
    return stream->has_pcm;
}

char *snd_state_str(snd_pcm_t *handle)
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

static snd_pcm_sframes_t stream_prefill_remaining(stream_t *stream)
{
    return stream->start_threshold - PREFILL_GAP - stream->prefilled;
}

static bool stream_prefilled(stream_t *stream)
{
    return stream_prefill_remaining(stream) > 0;
}

stream_flush_status_t stream_flush_pcm(stream_t *stream)
{
    assert(stream_has_pcm(stream));
    assert(stream_accepting_pcm(stream));

    printf("UIO SND|INFO: flushing\n");
    stream_debug_state(stream);

    snd_pcm_sframes_t frames_remaining = stream->pcm_copy.len / stream->frame_size - stream->pcm_offset;

    while (frames_remaining > 0) {

        snd_pcm_sframes_t to_write = frames_remaining;
        if (stream->state == STREAM_STATE_PAUSED) {
            to_write = min(stream_prefill_remaining(stream), to_write);
        }

        snd_pcm_sframes_t written = snd_pcm_writei(
            stream->handle, stream->pcm_data + stream->pcm_offset, to_write);

        if (written == -EAGAIN) {
            return STREAM_FLUSH_BLOCKED;
        } else if (written < 0) {
            printf("UIO SND|ERR: failed to flush pcm: %s, state %s\n", snd_strerror(written),
                   snd_state_str(stream->handle));

            // @alexbr: should we have an error state?
            stream->state = STREAM_STATE_PAUSED;
            return STREAM_FLUSH_ERR;
        }

        if (stream->state == STREAM_STATE_PAUSED) {
            stream->prefilled += written;
        }

        printf("UIO_SND|INFO: wrote %ld frames\n", written);

        stream->pcm_offset += written;
        frames_remaining -= written;
    }

    stream->has_pcm = false;
    return STREAM_FLUSH_NEED_MORE;
}

sddf_snd_pcm_data_t *stream_get_pcm(stream_t *stream)
{
    return &stream->pcm_copy;
}

bool stream_accepting_pcm(stream_t *stream)
{
    return (stream->state == STREAM_STATE_PAUSED && !stream_prefilled(stream)) ||
            stream->state == STREAM_STATE_PLAYING;
}

void stream_debug_state(stream_t *stream)
{
    printf("UIO SND|INFO: flushing, state %s, snd_state %s\n", stream_state_str(stream->state),
           snd_state_str(stream->handle));
}
