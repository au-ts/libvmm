/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <alsa/asoundlib.h>
#include <stdio.h>
 
unsigned char buffer[16*1024];

typedef struct wavfile_header_s
{
    char    ChunkID[4];
    int32_t ChunkSize;
    char    Format[4];
    
    char    Subchunk1ID[4];
    int32_t Subchunk1Size;
    int16_t AudioFormat;
    int16_t NumChannels;
    int32_t SampleRate;
    int32_t ByteRate;
    int16_t BlockAlign;
    int16_t BitsPerSample;
    
    char    Subchunk2ID[4];
    int32_t Subchunk2Size;
} wavfile_header_t;

#define NUM_CHANNELS 1
#define BITS_PER_SAMPLE 16
#define SUBCHUNK1SIZE 16

#define SAMPLE_RATE 48000

int write_header(FILE *file_p,
                 int32_t SampleRate,
                 int32_t FrameCount)
{
    int ret;
    
    wavfile_header_t wav_header;
    int32_t subchunk2_size;
    int32_t chunk_size;
    
    size_t write_count;
    
    subchunk2_size  = FrameCount * NUM_CHANNELS * BITS_PER_SAMPLE / 8;
    chunk_size      = 4 + (8 + SUBCHUNK1SIZE) + (8 + subchunk2_size);
    
    wav_header.ChunkID[0] = 'R';
    wav_header.ChunkID[1] = 'I';
    wav_header.ChunkID[2] = 'F';
    wav_header.ChunkID[3] = 'F';
    
    wav_header.ChunkSize = chunk_size;
    
    wav_header.Format[0] = 'W';
    wav_header.Format[1] = 'A';
    wav_header.Format[2] = 'V';
    wav_header.Format[3] = 'E';
    
    wav_header.Subchunk1ID[0] = 'f';
    wav_header.Subchunk1ID[1] = 'm';
    wav_header.Subchunk1ID[2] = 't';
    wav_header.Subchunk1ID[3] = ' ';
    
    wav_header.Subchunk1Size = SUBCHUNK1SIZE;
    wav_header.AudioFormat = 1;
    wav_header.NumChannels = NUM_CHANNELS;
    wav_header.SampleRate = SampleRate;
    wav_header.ByteRate = SampleRate * NUM_CHANNELS * BITS_PER_SAMPLE / 8;
    wav_header.BlockAlign = NUM_CHANNELS * BITS_PER_SAMPLE / 8;
    wav_header.BitsPerSample = BITS_PER_SAMPLE;
    
    wav_header.Subchunk2ID[0] = 'd';
    wav_header.Subchunk2ID[1] = 'a';
    wav_header.Subchunk2ID[2] = 't';
    wav_header.Subchunk2ID[3] = 'a';
    wav_header.Subchunk2Size = subchunk2_size;
    
    write_count = fwrite(&wav_header, 
                         sizeof(wavfile_header_t), 1,
                         file_p);
                    
    ret = (1 != write_count)? -1 : 0;
    
    return ret;
}

 
int main(int argc, char **argv)
{
    int err;
    unsigned int i;
    snd_pcm_t *handle;
    snd_pcm_sframes_t frames;

    char *device;
    if (argc > 1) {
        device = argv[1];
    } else {
        device = "default";
    }
 
    if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_CAPTURE, 0)) < 0) {
        printf("Capture open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }
    if ((err = snd_pcm_set_params(handle,
                      SND_PCM_FORMAT_S16,
                      SND_PCM_ACCESS_RW_INTERLEAVED,
                      1,
                      SAMPLE_RATE,
                      1,
                      500000)) < 0) {   /* 0.5sec */
        printf("Capture open error: %s\n", snd_strerror(err));
        exit(EXIT_FAILURE);
    }

    FILE *file = fopen("rec.wav", "wb");
    if (!file) {
        printf("Failed to open rec.wav\n");
        exit(EXIT_FAILURE);
    }

    const int buffer_count = 16;
    write_header(file, SAMPLE_RATE, sizeof(buffer) * buffer_count);

    const int frame_size = 2;
 
    for (i = 0; i < buffer_count; i++) {
        frames = snd_pcm_readi(handle, buffer, sizeof(buffer) / frame_size);
        if (frames < 0)
            frames = snd_pcm_recover(handle, frames, 0);
        if (frames < 0) {
            printf("snd_pcm_readi failed: %s\n", snd_strerror(frames));
            break;
        }
        if (frames > 0 && frames < (long)sizeof(buffer) / frame_size)
            printf("Short write (expected %li, wrote %li)\n", (long)sizeof(buffer) / frame_size, frames);

        fwrite(buffer, 1, sizeof(buffer), file);
    }
 
    /* pass the remaining samples, otherwise they're dropped in close */
    err = snd_pcm_drain(handle);
    if (err < 0)
        printf("snd_pcm_drain failed: %s\n", snd_strerror(err));
    snd_pcm_close(handle);

    fclose(file);
    return 0;
}

