#pragma once
#include "sddf_snd_shared_ringbuffer.h"
#include <alsa/asoundlib.h>
#include <stdbool.h>
#include <sys/poll.h>

typedef enum stream_state {
    STREAM_STATE_UNSET,
    STREAM_STATE_SET,
    STREAM_STATE_PAUSED,
    STREAM_STATE_PLAYING,
} stream_state_t;

typedef struct translation_state {
    ssize_t tx_offset;
    ssize_t rx_offset;
} translation_state_t;

typedef struct stream_response {
    sddf_snd_response_t response;
    sddf_snd_pcm_data_t tx_free;
    bool has_tx_free;
} stream_response_t;

typedef struct stream stream_t;

stream_t *stream_open(sddf_snd_pcm_info_t *info, const char *device, snd_pcm_stream_t direction,
                      translation_state_t translate);

void stream_enqueue_command(stream_t *stream, sddf_snd_command_t *cmd);

int stream_res_fd(stream_t *stream);
