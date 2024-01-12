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

typedef struct translation_state {
    ssize_t tx_offset;
    ssize_t rx_offset;
} translation_state_t;

typedef struct stream stream_t;

typedef void (*respond_fn)(uint32_t cookie, sddf_snd_status_code_t status, uint32_t latency_bytes,
                           void *user_data);
typedef void (*tx_free_fn)(uintptr_t addr, unsigned int len, void *user_data);

stream_t *stream_open(sddf_snd_pcm_info_t *info, const char *device, snd_pcm_stream_t stream,
                      translation_state_t translate, respond_fn respond, tx_free_fn tx_free,
                      void *user_data);

void stream_close(stream_t *stream);

void stream_enqueue_command(stream_t *stream, sddf_snd_command_t *cmd);

void stream_update(stream_t *stream);

bool stream_should_poll(stream_t *stream);

struct pollfd *stream_fds(stream_t *stream);

int stream_nfds(stream_t *stream);

int stream_demangle_fds(stream_t *stream, struct pollfd *pfds, unsigned int nfds,
                        unsigned short *revents);

snd_pcm_stream_t stream_direction(stream_t *stream);

void stream_debug_state(stream_t *stream);
