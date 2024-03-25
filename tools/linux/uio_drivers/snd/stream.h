#pragma once
#include <sddf/sound/queue.h>
#include <alsa/asoundlib.h>
#include <stdbool.h>

typedef struct stream stream_t;

stream_t *stream_open(sddf_snd_pcm_info_t *info, const char *device,
                      snd_pcm_stream_t direction, ssize_t translate_offset,
                      sddf_snd_cmd_ring_t *cmd_responses,
                      sddf_snd_pcm_data_ring_t *pcm_responses);

void stream_enqueue_command(stream_t *stream, sddf_snd_cmd_t *cmd);
void stream_enqueue_pcm_req(stream_t *stream, sddf_snd_pcm_data_t *pcm);

int stream_timer_fd(stream_t *stream);

/** Returns true to signal client notify */
bool stream_update(stream_t *stream);

snd_pcm_stream_t stream_direction(stream_t *stream);
