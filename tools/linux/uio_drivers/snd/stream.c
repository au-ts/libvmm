#include "stream.h"
#include "convert.h"
#include "log.h"
#include "queue.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/timerfd.h>
#include <time.h>

// This queue may have many TX buffers along side commands.
#define PCM_QUEUE_SIZE 8

// 0.5 seconds
#define LATENCY 500000
// Time between hardware interrupts
#define PERIOD_TIME 100000

#define BITS_PER_BYTE 8
#define NS_PER_SECOND 1000000000

typedef snd_pcm_sframes_t (*pcm_op_t)(stream_t *stream,
                                      void *pcm,
                                      snd_pcm_sframes_t to_read);

typedef enum stream_state {
    STREAM_STATE_UNSET,
    STREAM_STATE_SET,
    STREAM_STATE_PAUSED,
    STREAM_STATE_PLAYING,
    STREAM_STATE_IO_ERR,
    STREAM_STATE_DRAINING,
} stream_state_t;

struct stream {
    // ALSA
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *hw_params;
    snd_pcm_sw_params_t *sw_params;
    snd_pcm_stream_t direction;

    // Stream state
    stream_state_t state;
    ssize_t translate_offset;

    int frame_size;
    snd_pcm_sframes_t buffer_size;
    snd_pcm_sframes_t period_size;
    unsigned rate;

    queue_t *staged_responses;
    // Total frames played in audio stream
    snd_pcm_sframes_t consumed;
    // Offset of current buffer in total audio stream
    snd_pcm_sframes_t buffer_offset;
    int drain_count;

    bool timer_enabled;
    int timer_fd;

    // Communication
    queue_t *cmd_req;
    queue_t *pcm_req;

    sound_cmd_queue_t *cmd_res;
    sound_pcm_queue_t *pcm_res;
};

struct alsa_params {
    unsigned resample;
    snd_pcm_format_t format;
    unsigned channels;
    unsigned rate;
    unsigned latency_us;
    unsigned period_us; // time between hardware interrupts
};

struct buffer_state {
    snd_pcm_sframes_t buffer_size;
    snd_pcm_sframes_t period_size;
};


static const char *stream_state_str(stream_state_t state);
static char *snd_state_str(snd_pcm_t *handle);

static void print_err(int err, const char *msg)
{
    LOG_SOUND_ERR("%s: %s\n", msg, snd_strerror(err));
}

static snd_pcm_sframes_t min(snd_pcm_sframes_t a, snd_pcm_sframes_t b)
{
    return a <= b ? a : b;
}

static void *translate_addr(stream_t *stream, void *phys_addr)
{
    return phys_addr + stream->translate_offset;
}

static const char *stream_name(stream_t *stream)
{
    switch (stream->direction) {
    case SND_PCM_STREAM_CAPTURE:
        return "Recording";
    case SND_PCM_STREAM_PLAYBACK:
        return "Playback";
    default:
        return "<unknown>";
    }
}

static bool send_response(stream_t *stream)
{
    sound_pcm_t *response = queue_front(stream->staged_responses);
    if (!response) {
        return false;
    }

    response->latency_bytes = stream->buffer_size;
    response->status = stream->state == STREAM_STATE_IO_ERR ? SOUND_S_IO_ERR : SOUND_S_OK;

    if (sound_enqueue_pcm(stream->pcm_res, response) != 0) {
        LOG_SOUND_ERR("Failed to enqueue pcm_res\n");
        return false;
    }
    queue_dequeue(stream->staged_responses);
    return true;
}

static int stream_fail(stream_t *stream)
{
    LOG_SOUND("Failing stream\n");
    int responses_sent = 0;
    // Flush good responses.
    while (send_response(stream)) {
        responses_sent++;
    }

    // Respond to unhandled responses with error.
    while (!queue_empty(stream->pcm_req)) {

        sound_pcm_t *pcm = queue_front(stream->pcm_req);

        pcm->latency_bytes = stream->buffer_size;
        pcm->status = SOUND_S_IO_ERR;
        LOG_SOUND("Sending fail response cookie %d\n", pcm->cookie);

        if (sound_enqueue_pcm(stream->pcm_res, pcm) != 0) {
            LOG_SOUND_ERR("Failed to enqueue pcm_res\n");
            return responses_sent;
        }

        responses_sent++;
        queue_dequeue(stream->pcm_req);
    }

    stream->consumed = 0;
    stream->buffer_offset = 0;
    stream->state = STREAM_STATE_IO_ERR;

    return responses_sent;
}

/** Aligned memcpy implementation for device memory */
static void* device_copy(void *dst, void const *src, size_t len)
{
    long *l_dst = (long *)dst;
    long const *l_src = (long const *)src;

    if (((uintptr_t)src % sizeof(long) == 0) && ((uintptr_t)dst % sizeof(long) == 0)) {

        while (len >= sizeof(long)) {
            *l_dst++ = *l_src++;
            len -= sizeof(long);
        }
    }

    char *c_dst = (char *)l_dst;
    char const *c_src = (char const *)l_src;

    while (len--) {
        *c_dst++ = *c_src++;
    }

    return dst;
}

static snd_pcm_sframes_t stream_xfer(stream_t *stream,
                                     void *user_pcm,
                                     snd_pcm_sframes_t frames,
                                     bool write)
{
    const snd_pcm_channel_area_t *areas;
    snd_pcm_uframes_t alsa_offset;
    snd_pcm_uframes_t avail = (snd_pcm_uframes_t)frames;

    // We need to use mmap since user_pcm is in UIO "device" memory.
    int err = snd_pcm_mmap_begin(stream->handle, &areas, &alsa_offset, &avail);
    if (err == -EAGAIN) {
        return 0;
    } else if (err < 0) {
        LOG_SOUND_ERR("Failed to mmap pcm data: %s, state %s\n", snd_strerror(err),
                snd_state_str(stream->handle));
        return -1;
    }

    snd_pcm_sframes_t to_write = min((snd_pcm_sframes_t)avail, frames);
    size_t nbytes = to_write * stream->frame_size;

    void *alsa_pcm = areas[0].addr + (areas[0].first + alsa_offset * areas[0].step) / 8;

    if (write) {
        device_copy(alsa_pcm, user_pcm, nbytes);
    } else {
        device_copy(user_pcm, alsa_pcm, nbytes);
    }

    snd_pcm_sframes_t written
        = snd_pcm_mmap_commit(stream->handle, alsa_offset, to_write);

    if (written == -EAGAIN) {
        return 0;
    } else if (written < 0) {
        LOG_SOUND_ERR("Failed to flush pcm: %s, state %s\n", snd_strerror(written),
                snd_state_str(stream->handle));
        return -1;
    }

    return written;
}

static int next_buffer(stream_t *stream, sound_pcm_t *pcm)
{
    snd_pcm_sframes_t pcm_frames = pcm->len / stream->frame_size;

    stream->buffer_offset += pcm_frames;
    queue_enqueue(stream->staged_responses, pcm);
    queue_dequeue(stream->pcm_req);

    if (stream->state != STREAM_STATE_PAUSED) {
        if (!send_response(stream)) {
            LOG_SOUND_ERR("Failed to send response\n");
        }
        return 1;
    }
    return 0;
}

static int flush_pcm(stream_t *stream, int max_count)
{
    int response_count = 0;

    // @alexbr: for some reason this is needed for mmap to work
    snd_pcm_avail(stream->handle);

    while (!queue_empty(stream->pcm_req) && max_count-- > 0) {

        sound_pcm_t *pcm = queue_front(stream->pcm_req);

        snd_pcm_sframes_t pcm_frames = pcm->len / stream->frame_size;

        snd_pcm_sframes_t begin = stream->consumed - stream->buffer_offset;
        snd_pcm_sframes_t to_consume = pcm_frames - begin;

        if (to_consume <= 0) {
            response_count += next_buffer(stream, pcm);
            continue;
        }

        void *addr_offset = (void *)pcm->addr + (begin * stream->frame_size);
        void *pcm_data = translate_addr(stream, addr_offset);

        snd_pcm_sframes_t consumed = stream_xfer(stream, pcm_data, to_consume,
                                                 stream->direction == SND_PCM_STREAM_PLAYBACK);
        if (consumed < 0) {
            LOG_SOUND_ERR("Failed to read/write rx/tx\n");
            response_count += stream_fail(stream);
            break;
        }

        stream->consumed += consumed;

        if (consumed == to_consume) {
            response_count += next_buffer(stream, pcm);
        }

        if (consumed == 0) {
            break;
        }
    }

    if (stream->state == STREAM_STATE_DRAINING) {
        stream->drain_count -= response_count;
    }

    return response_count;
}

static sound_status_t set_hwparams(snd_pcm_t *handle, snd_pcm_hw_params_t *hw_params,
                                   snd_pcm_access_t access, struct alsa_params *params,
                                   struct buffer_state *state)
{
    unsigned int rrate;
    snd_pcm_uframes_t size;
    int err, dir;

    /* choose all parameters */
    err = snd_pcm_hw_params_any(handle, hw_params);
    if (err < 0) {
        LOG_SOUND_ERR("Broken configuration for playback: no configurations available: %s\n",
               snd_strerror(err));
        return SOUND_S_IO_ERR;
    }
    /* set hardware resampling */
    err = snd_pcm_hw_params_set_rate_resample(handle, hw_params, params->resample);
    if (err < 0) {
        LOG_SOUND_ERR("Resampling setup failed for playback: %s\n", snd_strerror(err));
        return SOUND_S_NOT_SUPP;
    }
    /* set the interleaved read/write format */
    err = snd_pcm_hw_params_set_access(handle, hw_params, access);
    if (err < 0) {
        LOG_SOUND_ERR("Access type not available for playback: %s\n", snd_strerror(err));
        return SOUND_S_NOT_SUPP;
    }
    /* set the sample format */
    err = snd_pcm_hw_params_set_format(handle, hw_params, params->format);
    if (err < 0) {
        LOG_SOUND_ERR("Sample format not available for playback: %s\n", snd_strerror(err));
        return SOUND_S_NOT_SUPP;
    }
    /* set the count of channels */
    err = snd_pcm_hw_params_set_channels(handle, hw_params, params->channels);
    if (err < 0) {
        LOG_SOUND_ERR("Channels count (%u) not available for playbacks: %s\n", params->channels,
               snd_strerror(err));
        return SOUND_S_NOT_SUPP;
    }
    /* set the stream rate */
    rrate = params->rate;
    err = snd_pcm_hw_params_set_rate_near(handle, hw_params, &rrate, 0);
    if (err < 0) {
        LOG_SOUND_ERR("Rate %uHz not available for playback: %s\n", params->rate, snd_strerror(err));
        return SOUND_S_NOT_SUPP;
    }
    if (rrate != params->rate) {
        LOG_SOUND_ERR("Rate doesn't match (requested %uHz, get %iHz)\n", params->rate, err);
        return SOUND_S_NOT_SUPP;
    }
    /* set the buffer time */
    err = snd_pcm_hw_params_set_buffer_time_near(handle, hw_params, &params->latency_us, &dir);
    if (err < 0) {
        LOG_SOUND_ERR("Unable to set buffer time %u for playback: %s\n", params->latency_us,
               snd_strerror(err));
        return SOUND_S_IO_ERR;
    }
    err = snd_pcm_hw_params_get_buffer_size(hw_params, &size);
    if (err < 0) {
        LOG_SOUND_ERR("Unable to get buffer size for playback: %s\n", snd_strerror(err));
        return SOUND_S_IO_ERR;
    }
    state->buffer_size = size;

    /* set the period time -- interval between consecutive interrupts from the sound card */
    err = snd_pcm_hw_params_set_period_time_near(handle, hw_params, &params->period_us, &dir);
    if (err < 0) {
        LOG_SOUND_ERR("Unable to set period time %u for playback: %s\n", params->period_us,
               snd_strerror(err));
        return SOUND_S_IO_ERR;
    }

    err = snd_pcm_hw_params_get_period_size(hw_params, &size, &dir);
    if (err < 0) {
        LOG_SOUND_ERR("Unable to set period size for playback: %s\n", snd_strerror(err));
        return SOUND_S_IO_ERR;
    }

    state->period_size = size;
    /* write the parameters to device */
    err = snd_pcm_hw_params(handle, hw_params);
    if (err < 0) {
        LOG_SOUND_ERR("Unable to set hw params for playback: %s\n", snd_strerror(err));
        return SOUND_S_NOT_SUPP;
    }
    return 0;
}

static sound_status_t set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams,
                                   struct buffer_state *state)
{
    int err;

    /* get the current swparams */
    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0) {
        LOG_SOUND_ERR("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* start the transfer when the buffer is almost full: */
    unsigned threshold = (state->buffer_size / state->period_size) * state->period_size;

    err = snd_pcm_sw_params_set_start_threshold(handle, swparams, threshold);
    if (err < 0) {
        LOG_SOUND_ERR("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
        return err;
    }

    /* allow the transfer when at least period_size samples can be processed */
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, state->period_size);
    if (err < 0) {
        LOG_SOUND_ERR("Unable to set avail min for playback: %s\n", snd_strerror(err));
        return err;
    }
    /* write the parameters to the playback device */
    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0) {
        LOG_SOUND_ERR("Unable to set sw params for playback: %s\n", snd_strerror(err));
        return err;
    }
    return 0;
}

static sound_status_t stream_set_params(stream_t *stream, sound_pcm_set_params_t *params)
{
    LOG_SOUND("[%s] Set parameters: format %s, rate %u, channels %d\n",
           stream_name(stream), sound_pcm_fmt_str(params->format),
           sddf_rate_to_alsa(params->rate), params->channels);

    bool state_valid =
        stream->state == STREAM_STATE_UNSET ||
        stream->state == STREAM_STATE_SET ||
        stream->state == STREAM_STATE_PAUSED;

    if (!state_valid) {
        LOG_SOUND_ERR("Cannot set params, invalid state\n");
        return SOUND_S_BAD_MSG;
    }

    snd_pcm_t *handle = stream->handle;

    // Set the sample format
    snd_pcm_format_t format = sddf_format_to_alsa(params->format);
    if (format == SND_PCM_FORMAT_UNKNOWN) {
        LOG_SOUND_ERR("Invalid format %d\n", params->format);
        return SOUND_S_NOT_SUPP;
    }

    unsigned rate = sddf_rate_to_alsa(params->rate);
    if (rate == INVALID_RATE) {
        LOG_SOUND_ERR("Invalid rate %d\n", params->rate);
        return SOUND_S_NOT_SUPP;
    }

    struct alsa_params alsa_params;
    alsa_params.channels = params->channels;
    alsa_params.format = format;
    alsa_params.latency_us = LATENCY;
    alsa_params.period_us = PERIOD_TIME;
    alsa_params.rate = rate;
    alsa_params.resample = 1;

    struct buffer_state buffer_state;
    sound_status_t code;

    code = set_hwparams(handle, stream->hw_params, SND_PCM_ACCESS_MMAP_INTERLEAVED, &alsa_params,
                        &buffer_state);

    if (code != SOUND_S_OK) {
        LOG_SOUND_ERR("Failed to set hardware params\n");
        return code;
    }

    code = set_swparams(handle, stream->sw_params, &buffer_state);

    if (code != SOUND_S_OK) {
        LOG_SOUND_ERR("Failed to set software params\n");
        return code;
    }

    stream->state = STREAM_STATE_SET;
    stream->frame_size = (snd_pcm_format_physical_width(format) / BITS_PER_BYTE) * params->channels;
    stream->buffer_size = buffer_state.buffer_size;
    stream->period_size = buffer_state.period_size;
    stream->rate = rate;

    return SOUND_S_OK;
}

static sound_status_t stream_prepare(stream_t *stream)
{
    if (stream->state != STREAM_STATE_SET && stream->state != STREAM_STATE_PAUSED) {
        LOG_SOUND_ERR("Cannot prepare in state '%s'\n", stream_state_str(stream->state));
        return SOUND_S_BAD_MSG;
    }

    int err = snd_pcm_prepare(stream->handle);
    if (err) {
        LOG_SOUND_ERR("Failed to prepare stream: %s\n", snd_strerror(err));
        return SOUND_S_IO_ERR;
    }

    LOG_SOUND("[%s] Prepared stream\n", stream_name(stream));
    stream->state = STREAM_STATE_PAUSED;

    stream->consumed = 0;
    stream->buffer_offset = 0;

    return SOUND_S_OK;
}

static sound_status_t stream_release(stream_t *stream)
{
    if (stream->state != STREAM_STATE_PAUSED && stream->state != STREAM_STATE_IO_ERR) {
        LOG_SOUND_ERR("Cannot release stream now, not paused\n");
        return SOUND_S_BAD_MSG;
    }

    LOG_SOUND("[%s] Released stream\n", stream_name(stream));
    stream->state = STREAM_STATE_SET;

    return SOUND_S_OK;
}

static sound_status_t timer_start(stream_t *stream)
{
    // Repeat every period, start immediately.
    struct itimerspec timer;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_nsec = (NS_PER_SECOND / (long)stream->rate) * stream->period_size;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_nsec = 1;

    if (timerfd_settime(stream->timer_fd, 0, &timer, NULL) != 0) {
        LOG_SOUND_ERR("Failed to set period timer\n");
        return SOUND_S_IO_ERR;
    }
    stream->timer_enabled = true;
    return SOUND_S_OK;
}

static sound_status_t timer_stop(stream_t *stream)
{
    // Finished drain, disable timer.
    struct itimerspec timer;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_nsec = 0;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_nsec = 0;
    if (timerfd_settime(stream->timer_fd, 0, &timer, NULL) != 0) {
        LOG_SOUND_ERR("Failed to set period timer\n");
        return SOUND_S_IO_ERR;
    }
    stream->timer_enabled = false;
    return SOUND_S_OK;
}

static sound_status_t stream_start(stream_t *stream)
{
    if (stream->state != STREAM_STATE_PAUSED) {
        LOG_SOUND_ERR("Cannot start stream now, invalid state\n");
        return SOUND_S_BAD_MSG;
    }

    LOG_SOUND("[%s] Starting stream\n", stream_name(stream));

    int err = snd_pcm_start(stream->handle);
    if (err < 0) {
        LOG_SOUND_ERR("Failed to start ALSA stream\n");
        return SOUND_S_IO_ERR;
    }

    sound_status_t status = timer_start(stream);

    // Drop any TX frames sent before START.
    if (stream->direction == SND_PCM_STREAM_PLAYBACK) {
        LOG_SOUND("[%s] Skipping %d early TX buffers\n", stream_name(stream),
               queue_size(stream->pcm_req));

        sound_pcm_t *pcm;
        while ((pcm = queue_front(stream->pcm_req))) {
            queue_enqueue(stream->staged_responses, pcm);
            queue_dequeue(stream->pcm_req);
        }
    }

    stream->state = STREAM_STATE_PLAYING;
    stream->buffer_offset = 0;

    return status;
}

static sound_status_t stream_stop(stream_t *stream, bool *blocked, bool *notify)
{
    bool state_valid =
        stream->state == STREAM_STATE_PLAYING ||
        stream->state == STREAM_STATE_DRAINING ||
        stream->state == STREAM_STATE_IO_ERR;

    if (!state_valid) {
        LOG_SOUND_ERR("Cannot stop stream at %s, invalid state\n",
               stream_state_str(stream->state));

        return SOUND_S_BAD_MSG;
    }

    // Initiate stop
    if (stream->state == STREAM_STATE_PLAYING) {
        stream->drain_count = queue_size(stream->pcm_req);
        stream->state = STREAM_STATE_DRAINING;
        LOG_SOUND("[%s] Stopping stream (draining %d buffers)\n", stream_name(stream),
               stream->drain_count);
    }

    if (stream->state != STREAM_STATE_IO_ERR && stream->drain_count > 0) {
        *blocked = true;
        return SOUND_S_OK;
    }

    int err;
    if (stream->direction == SND_PCM_STREAM_PLAYBACK) {
        err = snd_pcm_drain(stream->handle);
    } else {
        err = snd_pcm_drop(stream->handle);
    }

    if (err == -EAGAIN) {
        *blocked = true;
        return SOUND_S_OK;

    } else if (err < 0) {
        LOG_SOUND_ERR("Failed to drain stream: %s\n", snd_strerror(err));
        stream->state = STREAM_STATE_IO_ERR;
        return SOUND_S_IO_ERR;
    } else {
        // Flush remaining responses immediately
        while (send_response(stream))
            ;

        sound_status_t status = timer_stop(stream);

        if (status == SOUND_S_IO_ERR) {
            stream->state = STREAM_STATE_IO_ERR;
            return SOUND_S_IO_ERR;
        }

        stream->state = STREAM_STATE_PAUSED;
        LOG_SOUND("[%s] Stream stopped\n", stream_name(stream));
        return SOUND_S_OK;
    }
}

sound_status_t handle_command(stream_t *stream, sound_cmd_t *cmd,
                              bool *blocked, bool *notify)
{
    switch (cmd->code) {
    case SOUND_CMD_TAKE:
        return stream_set_params(stream, &cmd->set_params);
    case SOUND_CMD_PREPARE:
        return stream_prepare(stream);
    case SOUND_CMD_RELEASE:
        return stream_release(stream);
    case SOUND_CMD_START:
        return stream_start(stream);
    case SOUND_CMD_STOP:
        return stream_stop(stream, blocked, notify);
    default:
        LOG_SOUND_ERR("Unknown command %d\n", cmd->code);
        return SOUND_S_BAD_MSG;
    }
}

static bool stream_flush_commands(stream_t *stream)
{
    bool notify = false;

    sound_cmd_t *cmd;
    while ((cmd = queue_front(stream->cmd_req))) {

        bool blocked = false;
        sound_status_t status = handle_command(stream, cmd, &blocked, &notify);

        if (blocked) {
            break;
        }
        cmd->status = status;

        if (sound_enqueue_cmd(stream->cmd_res, cmd) != 0) {
            LOG_SOUND_ERR("Failed to enqueue response");
            break;
        }

        queue_dequeue(stream->cmd_req);
        notify = true;
    }

    return notify;
}

bool stream_update(stream_t *stream)
{
    int responses_sent = flush_pcm(stream, INT_MAX);

    bool notify = stream_flush_commands(stream);

    if (responses_sent > 0) {
        notify = true;
    }
    if (responses_sent == 0 && stream->state == STREAM_STATE_PLAYING) {
        if (send_response(stream)) {
            notify = true;
        }
    }
    return notify;
}

stream_t *stream_open(sound_pcm_info_t *info, const char *device, snd_pcm_stream_t direction,
                      ssize_t translate_offset, sound_cmd_queue_t *cmd_responses,
                      sound_pcm_queue_t *pcm_responses)
{
    stream_t *stream = malloc(sizeof(stream_t));
    if (stream == NULL) {
        LOG_SOUND_ERR("No enough memory\n");
        return NULL;
    }

    memset(info, 0, sizeof(sound_pcm_info_t));
    memset(stream, 0, sizeof(stream_t));

    int err;
    snd_pcm_t *handle = NULL;
    snd_pcm_hw_params_t *hw_params = NULL;
    snd_pcm_sw_params_t *sw_params = NULL;

    err = snd_pcm_open(&handle, device, direction, SND_PCM_NONBLOCK);
    if (err) {
        print_err(err, "Failed to open stream");
        goto fail;
    }

    err = snd_pcm_hw_params_malloc(&hw_params);
    if (err) {
        print_err(err, "Cannot allocate hardware parameter structure");
        goto fail;
    }

    err = snd_pcm_sw_params_malloc(&sw_params);
    if (err) {
        print_err(err, "Cannot allocate software parameter structure");
        goto fail;
    }

    err = snd_pcm_hw_params_any(handle, hw_params);
    if (err) {
        print_err(err, "Cannot initialize hardware parameter structure");
        goto fail;
    }

    unsigned int channels_min, channels_max;
    err = snd_pcm_hw_params_get_channels_min(hw_params, &channels_min);
    if (err) {
        print_err(err, "Cannot get min channels");
        goto fail;
    }

    err = snd_pcm_hw_params_get_channels_max(hw_params, &channels_max);
    if (err) {
        print_err(err, "Cannot get max channels");
        goto fail;
    }

    uint64_t rates = sddf_rates_from_hw_params(handle, hw_params);
    uint64_t formats = sddf_formats_from_hw_params(handle, hw_params);

    info->formats = formats;
    info->rates = rates;
    info->direction = direction == SND_PCM_STREAM_PLAYBACK ? SOUND_D_OUTPUT : SOUND_D_INPUT;
    info->channels_min = channels_min;
    info->channels_max = channels_max;

    stream->handle = handle;
    stream->state = STREAM_STATE_UNSET;
    stream->direction = direction;
    stream->hw_params = hw_params;
    stream->sw_params = sw_params;
    stream->translate_offset = translate_offset;

    stream->timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);

    stream->cmd_req = queue_create(sizeof(sound_cmd_t), SOUND_NUM_BUFFERS / 4);
    stream->cmd_res = cmd_responses;

    stream->pcm_req = queue_create(sizeof(sound_pcm_t), SOUND_NUM_BUFFERS / 4);
    stream->pcm_res = pcm_responses;

    stream->staged_responses = queue_create(sizeof(sound_pcm_t), PCM_QUEUE_SIZE);

    return stream;

fail:
    if (hw_params)
        snd_pcm_hw_params_free(hw_params);
    if (sw_params)
        snd_pcm_sw_params_free(sw_params);
    if (handle)
        snd_pcm_close(handle);
    if (stream)
        free(stream);
    return NULL;
}

void stream_enqueue_command(stream_t *stream, sound_cmd_t *cmd)
{
    queue_enqueue(stream->cmd_req, cmd);
}

void stream_enqueue_pcm_req(stream_t *stream, sound_pcm_t *pcm)
{
    queue_enqueue(stream->pcm_req, pcm);
}

int stream_timer_fd(stream_t *stream)
{
    return stream->timer_fd;
}

snd_pcm_stream_t stream_direction(stream_t *stream)
{
    return stream->direction;
}

static const char *stream_state_str(stream_state_t state)
{
    switch (state) {
    case STREAM_STATE_UNSET:
        return "unset";
    case STREAM_STATE_SET:
        return "set";
    case STREAM_STATE_PLAYING:
        return "playing";
    case STREAM_STATE_PAUSED:
        return "paused";
    case STREAM_STATE_DRAINING:
        return "draining";
    case STREAM_STATE_IO_ERR:
        return "IO ERROR";
    default:
        return "unknown";
    }
}

static char *snd_state_str(snd_pcm_t *handle)
{
    switch (snd_pcm_state(handle)) {
    case SND_PCM_STATE_OPEN:
        return "SND_PCM_STATE_OPEN";
        break;
    case SND_PCM_STATE_SETUP:
        return "SND_PCM_STATE_SETUP";
        break;
    case SND_PCM_STATE_PREPARED:
        return "SND_PCM_STATE_PREPARED";
        break;
    case SND_PCM_STATE_RUNNING:
        return "SND_PCM_STATE_RUNNING";
        break;
    case SND_PCM_STATE_XRUN:
        return "SND_PCM_STATE_XRUN";
        break;
    case SND_PCM_STATE_PAUSED:
        return "SND_PCM_STATE_PAUSED";
        break;
    case SND_PCM_STATE_SUSPENDED:
        return "SND_PCM_STATE_SUSPENDED";
        break;
    case SND_PCM_STATE_DISCONNECTED:
        return "SND_PCM_STATE_DISCONNECTED";
        break;
    default:
        return "unknown";
    }
}
