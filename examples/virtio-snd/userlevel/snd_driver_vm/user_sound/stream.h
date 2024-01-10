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
    STREAM_STATE_DRAINING,
} stream_state_t;

typedef enum stream_flush_status {
    STREAM_FLUSH_BLOCKED,
    STREAM_FLUSH_NEED_MORE,
    STREAM_FLUSH_ERR,
} stream_flush_status_t;

typedef struct stream stream_t;

stream_t *stream_open(sddf_snd_pcm_info_t *info, const char *device, snd_pcm_stream_t stream,
                      sddf_snd_ring_state_t *consume_ring);

void stream_close(stream_t *stream);

sddf_snd_status_code_t stream_set_params(stream_t *stream, sddf_snd_pcm_set_params_t *params);

sddf_snd_status_code_t stream_prepare(stream_t *stream);

sddf_snd_status_code_t stream_release(stream_t *stream);

sddf_snd_status_code_t stream_start(stream_t *stream);

sddf_snd_status_code_t stream_stop(stream_t *stream);

void stream_update(stream_t *stream);


bool stream_should_poll(stream_t *stream);

struct pollfd *stream_fds(stream_t *stream);

int stream_nfds(stream_t *stream);

int stream_demangle_fds(stream_t *stream, struct pollfd *pfds, unsigned int nfds,
                        unsigned short *revents);

snd_pcm_stream_t stream_direction(stream_t *stream);


void stream_set_pcm(stream_t *stream, sddf_snd_pcm_data_t *pcm, void *pcm_data);

sddf_snd_pcm_data_t *stream_get_pcm(stream_t *stream);

bool stream_has_pcm(stream_t *stream);

stream_flush_status_t stream_flush_pcm(stream_t *stream);

bool stream_accepting_pcm(stream_t *stream);

void stream_debug_state(stream_t *stream);
