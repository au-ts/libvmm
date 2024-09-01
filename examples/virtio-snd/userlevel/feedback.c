/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
 
unsigned char buffer[1024];

#define NUM_CHANNELS 1
#define SAMPLE_RATE 48000
#define FORMAT SND_PCM_FORMAT_S16
#define BITS_PER_BYTE 8

static bool running = true;

static snd_pcm_t *open_stream(snd_pcm_stream_t direction, const char *name, const char *device)
{
    int err;
    snd_pcm_t *handle;
    if ((err = snd_pcm_open(&handle, device, direction, 0)) < 0) {
        printf("%s open error: %s\n", name, snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    if ((err = snd_pcm_set_params(handle,
                      FORMAT,
                      SND_PCM_ACCESS_RW_INTERLEAVED,
                      1,
                      SAMPLE_RATE,
                      1,
                      500000)) < 0) {   /* 0.5sec */
        printf("Capture open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    printf("Opened %s\n", name);

    return handle;
}

void signal_handler(int signum)
{
    printf("\nStopping\n");
    running = false;
}
 
int main(int argc, char **argv)
{
    signal(SIGINT, signal_handler);

    snd_pcm_sframes_t frame_size = (snd_pcm_format_physical_width(FORMAT) / BITS_PER_BYTE) * NUM_CHANNELS;

    char *capture_device = argc > 1 ? argv[1] : "hw:0,1";

    snd_pcm_t *playback = open_stream(SND_PCM_STREAM_PLAYBACK, "Playback", "default");
    snd_pcm_t *capture = open_stream(SND_PCM_STREAM_CAPTURE, "Capture", capture_device);

    int err = snd_pcm_start(capture);
    if (err) {
        printf("Failed to start capture: %s\n", snd_strerror(err));
        return 1;
    }

    while (running) {
        snd_pcm_sframes_t read = snd_pcm_readi(capture, buffer, sizeof(buffer) / frame_size);
        if (read < 0) {
            printf("Recovering capture\n");
            read = snd_pcm_recover(capture, read, 0);
        }
        if (read < 0) {
            printf("snd_pcm_readi failed: %s\n", snd_strerror(read));
            break;
        }
        if (read > 0 && read < (long)sizeof(buffer) / frame_size)
            printf("Short read (expected %li, wrote %li)\n", (long)sizeof(buffer), read);

        snd_pcm_sframes_t written = snd_pcm_writei(playback, buffer, read);
        if (written < 0){
            printf("Recovering playback\n");
            written = snd_pcm_recover(playback, written, 0);
        }
        if (written < 0) {
            printf("snd_pcm_writteni failed: %s\n", snd_strerror(written));
            break;
        }
        if (written > 0 && written < (long)sizeof(buffer) / frame_size)
            printf("Short write (expected %li, wrote %li)\n", (long)sizeof(buffer), written);
    }
 
    /* pass the remaining samples, otherwise they're dropped in close */
    err = snd_pcm_drain(capture);
    if (err < 0)
        printf("snd_pcm_drain failed: %s\n", snd_strerror(err));
    snd_pcm_close(capture);

    err = snd_pcm_drain(playback);
    if (err < 0)
        printf("snd_pcm_drain failed: %s\n", snd_strerror(err));
    snd_pcm_close(playback);

    return 0;
}

