#include "stream.h"
#include <alsa/error.h>
#include <alsa/pcm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#define RATE_COUNT 14
#define FORMAT_COUNT 25

#define AVAIL_FRAMES 4
#define MIN_FRAMES 2

#define BITS_PER_BYTE 8

// 0.5 seconds
#define LATENCY 500000

#define INVALID_RATE ((unsigned)-1)

struct stream {
    snd_pcm_t *handle;
    snd_pcm_uframes_t period_size;
    snd_pcm_stream_t direction;
    stream_state_t state;
    sddf_snd_ring_state_t *consume_ring;
    int frame_size;

    bool has_pcm;
    sddf_snd_pcm_data_t pcm_copy;
    void *pcm_data;
    int pcm_offset;

    struct pollfd *fds;
    int nfds;
};

static unsigned int alsa_rates[RATE_COUNT] = {
    5512,  8000,  11025, 16000, 22050,  32000,  44100,
    48000, 64000, 88200, 96000, 176400, 192000, 384000,
};

static sddf_snd_pcm_rate_t sddf_rates[RATE_COUNT] = {
    SDDF_SND_PCM_RATE_5512,   SDDF_SND_PCM_RATE_8000,   SDDF_SND_PCM_RATE_11025,
    SDDF_SND_PCM_RATE_16000,  SDDF_SND_PCM_RATE_22050,  SDDF_SND_PCM_RATE_32000,
    SDDF_SND_PCM_RATE_44100,  SDDF_SND_PCM_RATE_48000,  SDDF_SND_PCM_RATE_64000,
    SDDF_SND_PCM_RATE_88200,  SDDF_SND_PCM_RATE_96000,  SDDF_SND_PCM_RATE_176400,
    SDDF_SND_PCM_RATE_192000, SDDF_SND_PCM_RATE_384000,
};

static snd_pcm_format_t alsa_formats[FORMAT_COUNT] = {
    SND_PCM_FORMAT_IMA_ADPCM,
    SND_PCM_FORMAT_MU_LAW,
    SND_PCM_FORMAT_A_LAW,
    SND_PCM_FORMAT_S8,
    SND_PCM_FORMAT_U8,
    SND_PCM_FORMAT_S16,
    SND_PCM_FORMAT_U16,
    SND_PCM_FORMAT_S18_3LE,
    SND_PCM_FORMAT_U18_3LE,
    SND_PCM_FORMAT_S20_3LE,
    SND_PCM_FORMAT_U20_3LE,
    SND_PCM_FORMAT_S24_3LE,
    SND_PCM_FORMAT_U24_3LE,
    SND_PCM_FORMAT_S20,
    SND_PCM_FORMAT_U20,
    SND_PCM_FORMAT_S24,
    SND_PCM_FORMAT_U24,
    SND_PCM_FORMAT_S32,
    SND_PCM_FORMAT_U32,
    SND_PCM_FORMAT_FLOAT,
    SND_PCM_FORMAT_FLOAT64,
    SND_PCM_FORMAT_DSD_U8,
    SND_PCM_FORMAT_DSD_U16_LE,
    SND_PCM_FORMAT_DSD_U32_LE,
    SND_PCM_FORMAT_IEC958_SUBFRAME,
};

static sddf_snd_pcm_fmt_t sddf_formats[FORMAT_COUNT] = {
    SDDF_SND_PCM_FMT_IMA_ADPCM,
    SDDF_SND_PCM_FMT_MU_LAW,
    SDDF_SND_PCM_FMT_A_LAW,
    SDDF_SND_PCM_FMT_S8,
    SDDF_SND_PCM_FMT_U8,
    SDDF_SND_PCM_FMT_S16,
    SDDF_SND_PCM_FMT_U16,
    SDDF_SND_PCM_FMT_S18_3,
    SDDF_SND_PCM_FMT_U18_3,
    SDDF_SND_PCM_FMT_S20_3,
    SDDF_SND_PCM_FMT_U20_3,
    SDDF_SND_PCM_FMT_S24_3,
    SDDF_SND_PCM_FMT_U24_3,
    SDDF_SND_PCM_FMT_S20,
    SDDF_SND_PCM_FMT_U20,
    SDDF_SND_PCM_FMT_S24,
    SDDF_SND_PCM_FMT_U24,
    SDDF_SND_PCM_FMT_S32,
    SDDF_SND_PCM_FMT_U32,
    SDDF_SND_PCM_FMT_FLOAT,
    SDDF_SND_PCM_FMT_FLOAT64,
    SDDF_SND_PCM_FMT_DSD_U8,
    SDDF_SND_PCM_FMT_DSD_U16,
    SDDF_SND_PCM_FMT_DSD_U32,
    SDDF_SND_PCM_FMT_IEC958_SUBFRAME,
};

static void print_err(int err, const char *msg)
{
    fprintf(stderr, "%s: %s\n", msg, snd_strerror(err));
}

static snd_pcm_format_t sddf_format_to_alsa(sddf_snd_pcm_fmt_t format)
{
    unsigned idx = (unsigned)format;
    if (idx >= FORMAT_COUNT)
        return SND_PCM_FORMAT_UNKNOWN;

    return alsa_formats[idx];
}

static unsigned int sddf_rate_to_alsa(sddf_snd_pcm_rate_t rate)
{
    unsigned idx = (unsigned)rate;
    if (idx >= RATE_COUNT)
        return INVALID_RATE;

    return alsa_rates[idx];
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

    uint64_t rates = 0;
    for (int i = 0; i < RATE_COUNT; i++) {
        if (snd_pcm_hw_params_test_rate(handle, hw_params, alsa_rates[i], 0) == 0) {
            rates |= (1 << sddf_rates[i]);
        }
    }

    uint64_t formats = 0;
    for (int i = 0; i < FORMAT_COUNT; i++) {
        if (snd_pcm_hw_params_test_format(handle, hw_params, alsa_formats[i]) == 0) {
            formats |= (1 << sddf_formats[i]);
        }
    }

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
    snd_pcm_hw_params_free(hw_params);

    return stream;

fail:
    if (fds)
        free(fds);
    if (hw_params)
        snd_pcm_hw_params_free(hw_params);
    if (handle)
        snd_pcm_close(handle);
    if (stream)
        free(stream);
    return NULL;
}

sddf_snd_status_code_t stream_set_params(stream_t *stream, sddf_snd_pcm_set_params_t *params)
{
    printf("UIO SND|INFO: set params stream %p, format %s, rate %u, channels %d, period_bytes %u\n",
           stream, sddf_snd_pcm_fmt_str(params->format), alsa_rates[params->rate], params->channels,
           params->period_bytes);

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

    int err = snd_pcm_set_params(handle, format, SND_PCM_ACCESS_RW_INTERLEAVED, params->channels,
                                 rate, 1, LATENCY);
    if (err) {
        printf("UIO SND|ERR: Failed to set params: %s\n", snd_strerror(err));
        return SDDF_SND_S_NOT_SUPP;
    }

    stream->state = STREAM_STATE_SET;
    stream->frame_size = (snd_pcm_format_physical_width(format) / BITS_PER_BYTE) * params->channels;

    // snd_pcm_sw_params_t *sw_params;
    // err = snd_pcm_sw_params_malloc(&sw_params);
    // if (err) {
    //     printf("UIO SND|ERR: failed to allocate sw params: %s\n", snd_strerror(err));
    //     return SDDF_SND_S_IO_ERR;
    // }

    // err = snd_pcm_sw_params_current(handle, sw_params);
    // if (err < 0) {
    //     printf("UIO SND|ERR: unable to determine current swparams for playback: %s\n", snd_strerror(err));
    //     snd_pcm_sw_params_free(sw_params);
    //     return err;
    // }

    // // Turn off autoplay
    // err = snd_pcm_sw_params_set_start_threshold(handle, sw_params, ULONG_MAX - 1);
    // if (err < 0) {
    //     printf("UIO SND|ERR: unable to set start threshold mode for playback: %s\n", snd_strerror(err));
    //     snd_pcm_sw_params_free(sw_params);
    //     return err;
    // }

    // err = snd_pcm_sw_params(handle, sw_params);
    // if (err < 0) {
    //     printf("UIO SND|ERR: unable to set sw params for playback: %s\n", snd_strerror(err));
    //     snd_pcm_sw_params_free(sw_params);
    //     return err;
    // }

    // snd_pcm_sw_params_free(sw_params);

    return SDDF_SND_S_OK;
}

sddf_snd_status_code_t stream_prepare(stream_t *stream)
{
    if (stream->state != STREAM_STATE_SET) {
        return SDDF_SND_S_BAD_MSG;
    }

    stream->state = STREAM_STATE_PAUSED;
    int err = snd_pcm_prepare(stream->handle);
    if (err) {
        printf("UIO SND|ERR: Failed to prepare stream: %s\n", snd_strerror(err));
        return SDDF_SND_S_IO_ERR;
    }

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
    while ((err = snd_pcm_drain(stream->handle) == EAGAIN))
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

stream_flush_status_t stream_flush_pcm(stream_t *stream)
{
    assert(stream_has_pcm(stream));
    assert(stream_accepting_pcm(stream));
    printf("UIO SND|INFO: flushing\n");
    stream_debug_state(stream);

    int pcm_remaining = stream->pcm_copy.len - stream->pcm_offset;
    while (pcm_remaining > 0) {
        snd_pcm_sframes_t err
            = snd_pcm_writei(stream->handle, stream->pcm_data + stream->pcm_offset, pcm_remaining);
        if (err == -EAGAIN) {
            return STREAM_FLUSH_BLOCKED;
        } else if (err < 0) {
            printf("UIO SND|ERR: failed to flush pcm: %s, state %s\n", snd_strerror(err),
                   snd_state_str(stream->handle));

            // @alexbr: should we have an error state?
            stream->state = STREAM_STATE_PAUSED;
            return STREAM_FLUSH_ERR;
        }
        int written = err * stream->frame_size;
        printf("UIO_SND|INFO: wrote %d\n", written);

        stream->pcm_offset += written;
        pcm_remaining -= written;
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
    return stream->state == STREAM_STATE_PAUSED || stream->state == STREAM_STATE_PLAYING;
}

void stream_debug_state(stream_t *stream)
{
    printf("UIO SND|INFO: flushing, state %s, snd_state %s\n", stream_state_str(stream->state),
           snd_state_str(stream->handle));
}
