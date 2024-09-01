/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sddf/sound/queue.h>
#include <alsa/asoundlib.h>

#define INVALID_RATE ((unsigned)-1)

uint64_t sddf_rates_from_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);

uint64_t sddf_formats_from_hw_params(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);

snd_pcm_format_t sddf_format_to_alsa(sound_pcm_fmt_t format);

unsigned int sddf_rate_to_alsa(sound_pcm_rate_t rate);
