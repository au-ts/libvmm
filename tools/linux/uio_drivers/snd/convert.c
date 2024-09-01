/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "convert.h"

#define RATE_COUNT 14
#define FORMAT_COUNT 25

static unsigned int alsa_rates[RATE_COUNT] = {
    5512,  8000,  11025, 16000, 22050,  32000,  44100,
    48000, 64000, 88200, 96000, 176400, 192000, 384000,
};

static sound_pcm_rate_t sddf_rates[RATE_COUNT] = {
    SOUND_PCM_RATE_5512,   SOUND_PCM_RATE_8000,   SOUND_PCM_RATE_11025,
    SOUND_PCM_RATE_16000,  SOUND_PCM_RATE_22050,  SOUND_PCM_RATE_32000,
    SOUND_PCM_RATE_44100,  SOUND_PCM_RATE_48000,  SOUND_PCM_RATE_64000,
    SOUND_PCM_RATE_88200,  SOUND_PCM_RATE_96000,  SOUND_PCM_RATE_176400,
    SOUND_PCM_RATE_192000, SOUND_PCM_RATE_384000,
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

static sound_pcm_fmt_t sddf_formats[FORMAT_COUNT] = {
    SOUND_PCM_FMT_IMA_ADPCM,
    SOUND_PCM_FMT_MU_LAW,
    SOUND_PCM_FMT_A_LAW,
    SOUND_PCM_FMT_S8,
    SOUND_PCM_FMT_U8,
    SOUND_PCM_FMT_S16,
    SOUND_PCM_FMT_U16,
    SOUND_PCM_FMT_S18_3,
    SOUND_PCM_FMT_U18_3,
    SOUND_PCM_FMT_S20_3,
    SOUND_PCM_FMT_U20_3,
    SOUND_PCM_FMT_S24_3,
    SOUND_PCM_FMT_U24_3,
    SOUND_PCM_FMT_S20,
    SOUND_PCM_FMT_U20,
    SOUND_PCM_FMT_S24,
    SOUND_PCM_FMT_U24,
    SOUND_PCM_FMT_S32,
    SOUND_PCM_FMT_U32,
    SOUND_PCM_FMT_FLOAT,
    SOUND_PCM_FMT_FLOAT64,
    SOUND_PCM_FMT_DSD_U8,
    SOUND_PCM_FMT_DSD_U16,
    SOUND_PCM_FMT_DSD_U32,
    SOUND_PCM_FMT_IEC958_SUBFRAME,
};

uint64_t sddf_rates_from_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
    uint64_t rates = 0;
    for (int i = 0; i < RATE_COUNT; i++) {
        if (snd_pcm_hw_params_test_rate(pcm, params, alsa_rates[i], 0) == 0) {
            rates |= (1 << sddf_rates[i]);
        }
    }
    return rates;
}

uint64_t sddf_formats_from_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params)
{
    uint64_t formats = 0;
    for (int i = 0; i < FORMAT_COUNT; i++) {
        if (snd_pcm_hw_params_test_format(pcm, params, alsa_formats[i]) == 0) {
            formats |= (1 << sddf_formats[i]);
        }
    }
    return formats;
}

snd_pcm_format_t sddf_format_to_alsa(sound_pcm_fmt_t format)
{
    unsigned idx = (unsigned)format;
    if (idx >= FORMAT_COUNT)
        return SND_PCM_FORMAT_UNKNOWN;

    return alsa_formats[idx];
}

unsigned int sddf_rate_to_alsa(sound_pcm_rate_t rate)
{
    unsigned idx = (unsigned)rate;
    if (idx >= RATE_COUNT)
        return INVALID_RATE;

    return alsa_rates[idx];
}
