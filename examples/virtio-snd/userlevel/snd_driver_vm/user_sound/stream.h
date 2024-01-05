#pragma once
#include "sddf_snd_shared_ringbuffer.h"
#include <alsa/asoundlib.h>
#include <sys/poll.h>

typedef enum stream_state {
    STREAM_STATE_UNSET,
    STREAM_STATE_SET,
    STREAM_STATE_PAUSED,
    STREAM_STATE_PLAYING,
} stream_state_t;

typedef struct stream stream_t;

stream_t *stream_open(sddf_snd_pcm_info_t *info, const char *device,
                snd_pcm_stream_t stream);

sddf_snd_status_code_t stream_set_params(stream_t *stream, sddf_snd_pcm_set_params_t *params);

struct pollfd *stream_fds(stream_t *stream);
int stream_nfds(stream_t *stream);
