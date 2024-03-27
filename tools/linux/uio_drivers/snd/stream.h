#pragma once
#include <sddf/sound/queue.h>
#include <alsa/asoundlib.h>
#include <stdbool.h>

typedef struct stream stream_t;

stream_t *stream_open(sound_pcm_info_t *info, const char *device,
                      snd_pcm_stream_t direction, ssize_t translate_offset,
                      sound_cmd_queue_t *cmd_responses,
                      sound_pcm_queue_t *pcm_responses);

void stream_enqueue_command(stream_t *stream, sound_cmd_t *cmd);
void stream_enqueue_pcm_req(stream_t *stream, sound_pcm_t *pcm);

int stream_timer_fd(stream_t *stream);

/** Returns true to signal client notify */
bool stream_update(stream_t *stream);

snd_pcm_stream_t stream_direction(stream_t *stream);
