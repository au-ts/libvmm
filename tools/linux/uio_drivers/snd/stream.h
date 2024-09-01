/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <sddf/sound/queue.h>
#include <alsa/asoundlib.h>
#include <stdbool.h>

typedef struct stream stream_t;

stream_t *stream_open(sound_pcm_info_t *info,
                      const char *device,
                      snd_pcm_stream_t direction,
                      ssize_t translate_offset,
                      sound_cmd_queue_handle_t *cmd_res,
                      sound_pcm_queue_handle_t *pcm_res);

void stream_enqueue_command(stream_t *stream, sound_cmd_t *cmd);
void stream_enqueue_pcm_req(stream_t *stream, sound_pcm_t *pcm);

int stream_timer_fd(stream_t *stream);

/* Returns true to signal client notify */
bool stream_update(stream_t *stream);

snd_pcm_stream_t stream_direction(stream_t *stream);
