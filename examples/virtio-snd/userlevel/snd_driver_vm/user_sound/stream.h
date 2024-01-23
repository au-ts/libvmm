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
                      translation_state_t translate);

int stream_enqueue_command(stream_t *stream, sddf_snd_command_t *cmd);
int stream_dequeue_response(stream_t *stream, sddf_snd_response_t *res);

int stream_enqueue_tx_used(stream_t *stream, sddf_snd_pcm_data_t *pcm);
int stream_dequeue_tx_free(stream_t *stream, sddf_snd_pcm_data_t *pcm);

int stream_enqueue_rx_free(stream_t *stream, sddf_snd_pcm_data_t *pcm);
int stream_dequeue_rx_used(stream_t *stream, sddf_snd_pcm_data_t *pcm);

int stream_notify(stream_t *stream);
int stream_response_fd(stream_t *stream);

int stream_res_fd(stream_t *stream);
