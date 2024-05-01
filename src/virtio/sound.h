/*
 * Copyright 2019, DornerWorks
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/* This header is BSD licensed so anyone can use the definitions to implement
 * compatible drivers/servers.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of IBM nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE. */

#pragma once

#include <stdint.h>
#include <sddf/sound/queue.h>
#include "virtio/mmio.h"
#include "util/queue.h"
#include "util/buffer.h"

#define VIRTIO_SND_NUM_VIRTQ 4

struct virtio_snd_config {
    uint32_t jacks;
    uint32_t streams;
    uint32_t chmaps;
} __attribute__((packed));

enum {
    /* jack control request types */
    VIRTIO_SND_R_JACK_INFO = 1,
    VIRTIO_SND_R_JACK_REMAP,
    /* PCM control request types */
    VIRTIO_SND_R_PCM_INFO = 0x0100,
    VIRTIO_SND_R_PCM_SET_PARAMS,
    VIRTIO_SND_R_PCM_PREPARE,
    VIRTIO_SND_R_PCM_RELEASE,
    VIRTIO_SND_R_PCM_START,
    VIRTIO_SND_R_PCM_STOP,
    /* channel map control request types */
    VIRTIO_SND_R_CHMAP_INFO = 0x0200,
    /* jack event types */
    VIRTIO_SND_EVT_JACK_CONNECTED = 0x1000,
    VIRTIO_SND_EVT_JACK_DISCONNECTED,
    /* PCM event types */
    VIRTIO_SND_EVT_PCM_PERIOD_ELAPSED = 0x1100,
    VIRTIO_SND_EVT_PCM_XRUN,
    /* common status codes */
    VIRTIO_SOUND_S_OK = 0x8000,
    VIRTIO_SOUND_S_BAD_MSG,
    VIRTIO_SOUND_S_NOT_SUPP,
    VIRTIO_SOUND_S_IO_ERR
};

// Common header
struct virtio_snd_hdr {
    uint32_t code;
};

// An event notification
struct virtio_snd_event {
    struct virtio_snd_hdr hdr;
    uint32_t data;
};

enum {
    VIRTIO_SND_D_OUTPUT = 0,
    VIRTIO_SND_D_INPUT
};

// VIRTIO_SND_R_*_INFO control request
struct virtio_snd_query_info {
    struct virtio_snd_hdr hdr;
    // Start index into respective jack/stream/chmap array
    uint32_t start_id;
    // Number of items we want info about
    uint32_t count;
    // sizeof(virtio_snd_*_info)
    uint32_t size;
};

struct virtio_snd_info {
    uint32_t hda_fn_nid;
};

//// JACK ////
// VIRTIO_SND_R_JACK_* control request header
struct virtio_snd_jack_hdr {
    struct virtio_snd_hdr hdr;
    uint32_t jack_id;
};

// Supported jack features
enum {
    VIRTIO_SND_JACK_F_REMAP = 0
};

// VIRTIO_SND_R_JACK_INFO response
struct virtio_snd_jack_info {
    struct virtio_snd_info hdr;
    uint32_t features; /* 1 << VIRTIO_SND_JACK_F_XXX */
    uint32_t hda_reg_defconf;
    uint32_t hda_reg_caps;
    uint8_t connected;
    uint8_t padding[7];
} __attribute__((packed));

// VIRTIO_SND_R_JACK_REMAP request
struct virtio_snd_jack_remap {
    struct virtio_snd_jack_hdr hdr; /* .code = VIRTIO_SND_R_JACK_REMAP */
    uint32_t association;
    uint32_t sequence;
};

//// PCM ////
// PCM control request
struct virtio_snd_pcm_hdr {
    struct virtio_snd_hdr hdr;
    uint32_t stream_id;
};

// Supported PCM stream features
enum {
    VIRTIO_SND_PCM_F_SHMEM_HOST = 0,
    VIRTIO_SND_PCM_F_SHMEM_GUEST,
    VIRTIO_SND_PCM_F_MSG_POLLING,
    VIRTIO_SND_PCM_F_EVT_SHMEM_PERIODS,
    VIRTIO_SND_PCM_F_EVT_XRUNS
};

// Supported PCM sample formats
enum {
    // Analog formats (width / physical width)
    VIRTIO_SND_PCM_FMT_IMA_ADPCM = 0, /* 4 / 4 bits */
    VIRTIO_SND_PCM_FMT_MU_LAW, /* 8 / 8 bits */
    VIRTIO_SND_PCM_FMT_A_LAW, /* 8 / 8 bits */
    VIRTIO_SND_PCM_FMT_S8, /* 8 / 8 bits */
    VIRTIO_SND_PCM_FMT_U8, /* 8 / 8 bits */
    VIRTIO_SND_PCM_FMT_S16, /* 16 / 16 bits */
    VIRTIO_SND_PCM_FMT_U16, /* 16 / 16 bits */
    VIRTIO_SND_PCM_FMT_S18_3, /* 18 / 24 bits */
    VIRTIO_SND_PCM_FMT_U18_3, /* 18 / 24 bits */
    VIRTIO_SND_PCM_FMT_S20_3, /* 20 / 24 bits */
    VIRTIO_SND_PCM_FMT_U20_3, /* 20 / 24 bits */
    VIRTIO_SND_PCM_FMT_S24_3, /* 24 / 24 bits */
    VIRTIO_SND_PCM_FMT_U24_3, /* 24 / 24 bits */
    VIRTIO_SND_PCM_FMT_S20, /* 20 / 32 bits */
    VIRTIO_SND_PCM_FMT_U20, /* 20 / 32 bits */
    VIRTIO_SND_PCM_FMT_S24, /* 24 / 32 bits */
    VIRTIO_SND_PCM_FMT_U24, /* 24 / 32 bits */
    VIRTIO_SND_PCM_FMT_S32, /* 32 / 32 bits */
    VIRTIO_SND_PCM_FMT_U32, /* 32 / 32 bits */
    VIRTIO_SND_PCM_FMT_FLOAT, /* 32 / 32 bits */
    VIRTIO_SND_PCM_FMT_FLOAT64, /* 64 / 64 bits */
    // Digital formats (width / physical width)
    VIRTIO_SND_PCM_FMT_DSD_U8, /* 8 / 8 bits */
    VIRTIO_SND_PCM_FMT_DSD_U16, /* 16 / 16 bits */
    VIRTIO_SND_PCM_FMT_DSD_U32, /* 32 / 32 bits */
    VIRTIO_SND_PCM_FMT_IEC958_SUBFRAME /* 32 / 32 bits */
};

// Supported PCM frame rates
enum {
    VIRTIO_SND_PCM_RATE_5512 = 0,
    VIRTIO_SND_PCM_RATE_8000,
    VIRTIO_SND_PCM_RATE_11025,
    VIRTIO_SND_PCM_RATE_16000,
    VIRTIO_SND_PCM_RATE_22050,
    VIRTIO_SND_PCM_RATE_32000,
    VIRTIO_SND_PCM_RATE_44100,
    VIRTIO_SND_PCM_RATE_48000,
    VIRTIO_SND_PCM_RATE_64000,
    VIRTIO_SND_PCM_RATE_88200,
    VIRTIO_SND_PCM_RATE_96000,
    VIRTIO_SND_PCM_RATE_176400,
    VIRTIO_SND_PCM_RATE_192000,
    VIRTIO_SND_PCM_RATE_384000
};

// VIRTIO_SND_R_PCM_INFO response
struct virtio_snd_pcm_info { // largest at 32 bytes
    struct virtio_snd_info hdr;
    uint32_t features; /* 1 << VIRTIO_SND_PCM_F_XXX */
    uint64_t formats; /* 1 << VIRTIO_SND_PCM_FMT_XXX */
    uint64_t rates; /* 1 << VIRTIO_SND_PCM_RATE_XXX */
    uint8_t direction;
    uint8_t channels_min;
    uint8_t channels_max;
    uint8_t padding[5];
} __attribute__((packed));

// VIRTIO_SND_R_PCM_SET_PARAMS request
struct virtio_snd_pcm_set_params {
    struct virtio_snd_pcm_hdr hdr; /* .code = VIRTIO_SND_R_PCM_SET_PARAMS */
    uint32_t buffer_bytes;
    uint32_t period_bytes;
    uint32_t features; /* 1 << VIRTIO_SND_PCM_F_XXX */
    uint8_t channels;
    uint8_t format;
    uint8_t rate;
    uint8_t padding;
} __attribute__((packed));

/* an I/O header */
struct virtio_snd_pcm_xfer {
    uint32_t stream_id;
};
/* an I/O status */
struct virtio_snd_pcm_status {
    uint32_t status;
    uint32_t latency_bytes;
};

// Standard channel position definition
enum {
    VIRTIO_SND_CHMAP_NONE = 0, /* undefined */
    VIRTIO_SND_CHMAP_NA, /* silent */
    VIRTIO_SND_CHMAP_MONO, /* mono stream */
    VIRTIO_SND_CHMAP_FL, /* front left */
    VIRTIO_SND_CHMAP_FR, /* front right */
    VIRTIO_SND_CHMAP_RL, /* rear left */
    VIRTIO_SND_CHMAP_RR, /* rear right */
    VIRTIO_SND_CHMAP_FC, /* front center */
    VIRTIO_SND_CHMAP_LFE, /* low frequency (LFE) */
    VIRTIO_SND_CHMAP_SL, /* side left */
    VIRTIO_SND_CHMAP_SR, /* side right */
    VIRTIO_SND_CHMAP_RC, /* rear center */
    VIRTIO_SND_CHMAP_FLC, /* front left center */
    VIRTIO_SND_CHMAP_FRC, /* front right center */
    VIRTIO_SND_CHMAP_RLC, /* rear left center */
    VIRTIO_SND_CHMAP_RRC, /* rear right center */
    VIRTIO_SND_CHMAP_FLW, /* front left wide */
    VIRTIO_SND_CHMAP_FRW, /* front right wide */
    VIRTIO_SND_CHMAP_FLH, /* front left high */
    VIRTIO_SND_CHMAP_FCH, /* front center high */
    VIRTIO_SND_CHMAP_FRH, /* front right high */
    VIRTIO_SND_CHMAP_TC, /* top center */
    VIRTIO_SND_CHMAP_TFL, /* top front left */
    VIRTIO_SND_CHMAP_TFR, /* top front right */
    VIRTIO_SND_CHMAP_TFC, /* top front center */
    VIRTIO_SND_CHMAP_TRL, /* top rear left */
    VIRTIO_SND_CHMAP_TRR, /* top rear right */
    VIRTIO_SND_CHMAP_TRC, /* top rear center */
    VIRTIO_SND_CHMAP_TFLC, /* top front left center */
    VIRTIO_SND_CHMAP_TFRC, /* top front right center */
    VIRTIO_SND_CHMAP_TSL, /* top side left */
    VIRTIO_SND_CHMAP_TSR, /* top side right */
    VIRTIO_SND_CHMAP_LLFE, /* left LFE */
    VIRTIO_SND_CHMAP_RLFE, /* right LFE */
    VIRTIO_SND_CHMAP_BC, /* bottom center */
    VIRTIO_SND_CHMAP_BLC, /* bottom left center */
    VIRTIO_SND_CHMAP_BRC /* bottom right center */
};

// Maximum possible number of channels
#define VIRTIO_SND_CHMAP_MAX_SIZE 18

struct virtio_snd_chmap_info {
    struct virtio_snd_info hdr;
    // VIRTIO_SND_D_*
    uint8_t direction; 
    // The number of valid channel position values
    uint8_t channels;  
    // Channel position values (VIRTIO_SND_CHMAP_*)
    uint8_t positions[VIRTIO_SND_CHMAP_MAX_SIZE];
};

typedef struct virtio_snd_request {
    uint32_t cookie;
    uint16_t desc_head;
    uint16_t ref_count;
    uint16_t status;
    uint16_t virtq_idx;
    // RX only
    uint32_t bytes_received;
} virtio_snd_request_t;

#define VIRTIO_SND_MAX_CMD_REQUESTS 6
#define VIRTIO_SND_MAX_PCM_REQUESTS 32

struct virtio_snd_device {
    struct virtio_device virtio_device;

    struct virtio_snd_config config;
    struct virtio_queue_handler vqs[VIRTIO_SND_NUM_VIRTQ];
    // Only one command can be in-flight at a time.
    // Queue of virtio_snd_request_t
    // PCM requests must be responded to in order.
    queue_t cmd_requests;
    buffer_t cmd_requests_data[VIRTIO_SND_MAX_CMD_REQUESTS];
    // Queue of virtio_snd_request_t
    // PCM requests must be responded to in order.
    queue_t pcm_requests;
    buffer_t pcm_requests_data[VIRTIO_SND_MAX_PCM_REQUESTS];
    uint32_t curr_cookie;
    // Queue of buffer_t structs
    queue_t free_buffers;
    buffer_t free_buffers_data[SOUND_NUM_BUFFERS];
    // sDDF state
    sound_shared_state_t *shared_state;
    sound_queues_t queues;
    int server_ch;
};

/**
 * Initialise a virtIO sound device with MMIO.
 * This must be called *after* driver has finished initialisation.
 * 
 * @param sound_dev Preallocated memory for the sound device,
 *                  does not need to be pre-initialised.
 * @param region_base Start of the MMIO fault region.
 * @param region_size Size of the MMIO fault region.
 * @param virq The virtual IRQ used to notify the VM.
 * @param shared_state Pointer to the sDDF sound shared data region.
 * @param queues Pointer to sDDF sound queues.
 * @param server_ch Channel of the sound server.
 * 
 * @return `true` on success, `false` otherwise.
 */
bool virtio_mmio_snd_init(struct virtio_snd_device *sound_dev,
                     uintptr_t region_base,
                     uintptr_t region_size,
                     size_t virq,
                     sound_shared_state_t *shared_state,
                     sound_queues_t *queues,
                     int server_ch);

void virtio_snd_notified(struct virtio_snd_device *sound_dev);
