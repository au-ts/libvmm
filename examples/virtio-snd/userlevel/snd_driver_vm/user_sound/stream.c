#include "stream.h"
#include <stdio.h>
#include <stdlib.h>

#define RATE_COUNT 14
#define FORMAT_COUNT 25

#define INVALID_RATE ((unsigned)-1)

struct stream {
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_uframes_t period_size;
    stream_state_t state;

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

static void print_bytes(void *data)
{
    for (int i = 0; i < 16; i++) {
        printf("%02x ", ((uint8_t *)data)[i]);
    }
    printf("\n");
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

stream_t *stream_open(sddf_snd_pcm_info_t *info, const char *device, snd_pcm_stream_t direction)
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
    stream->hw_params = hw_params;
    stream->fds = fds;
    stream->nfds = nfds;
    stream->state = STREAM_STATE_UNSET;
    // state->period_size = period_size;

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
    printf("UIO SND|INFO: set params stream %p, format %s, rate %u, channels %d\n", stream,
           sddf_snd_pcm_fmt_str(params->format), alsa_rates[params->rate], params->channels);

    int err;
    snd_pcm_t *handle = stream->handle;
    snd_pcm_hw_params_t *hw_params = stream->hw_params;

    // @alexbr: from alsa pcm.c example write_and_poll
    err = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    if (err < 0) {
        printf("Access type not available for playback: %s\n", snd_strerror(err));
        return SDDF_SND_S_NOT_SUPP;
    }

    // Set the sample format
    snd_pcm_format_t format = sddf_format_to_alsa(params->format);
    if (format == SND_PCM_FORMAT_UNKNOWN) {
        printf("Invalid format %d\n", params->format);
        return SDDF_SND_S_NOT_SUPP;
    }

    err = snd_pcm_hw_params_set_format(handle, hw_params, format);
    if (err < 0) {
        printf("Sample format not available for playback: %s\n", snd_strerror(err));
        return SDDF_SND_S_NOT_SUPP;
    }

    // Set the count of channels
    err = snd_pcm_hw_params_set_channels(handle, hw_params, params->channels);
    if (err < 0) {
        printf("Channels count (%u) not available: %s\n", params->channels,
               snd_strerror(err));
        return SDDF_SND_S_NOT_SUPP;
    }

    unsigned rate = sddf_rate_to_alsa(params->rate);
    if (rate == INVALID_RATE) {
        printf("Invalid rate %d\n", params->rate);
        return SDDF_SND_S_NOT_SUPP;
    }

    // Set the stream rate
    unsigned rrate = rate;
    err = snd_pcm_hw_params_set_rate_near(handle, hw_params, &rrate, 0);
    if (err < 0) {
        printf("Rate %uHz not available for playback: %s\n", rate, snd_strerror(err));
        return SDDF_SND_S_NOT_SUPP;
    }
    if (rrate != rate) {
        printf("Rate doesn't match (requested %uHz, get %iHz)\n", rate, err);
        return SDDF_SND_S_NOT_SUPP;
    }

    // TODO: might need to set rate_near, buffer_time_near, period_time_near

    stream->state = STREAM_STATE_SET;

    return SDDF_SND_S_OK;
}

struct pollfd *stream_fds(stream_t *stream)
{
    return stream->fds;
}

int stream_nfds(stream_t *stream)
{
    return stream->nfds;
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
