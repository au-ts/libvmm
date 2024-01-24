#pragma once
#include "sddf_snd_shared_ringbuffer.h"
#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <sys/poll.h>

typedef struct translation_state {
    ssize_t tx_offset;
    ssize_t rx_offset;
} translation_state_t;

typedef struct stream stream_t;

stream_t *stream_open(sddf_snd_pcm_info_t *info, const char *device, snd_pcm_stream_t direction,
                      ssize_t translate_offset);

int stream_enqueue_command(stream_t *stream, sddf_snd_command_t *cmd);
int stream_dequeue_response(stream_t *stream, sddf_snd_response_t *res);

int stream_enqueue_pcm_req(stream_t *stream, sddf_snd_pcm_data_t *pcm);
int stream_dequeue_pcm_res(stream_t *stream, sddf_snd_pcm_data_t *pcm);

int stream_notify(stream_t *stream);
int stream_response_fd(stream_t *stream);

snd_pcm_stream_t stream_direction(stream_t *stream);
