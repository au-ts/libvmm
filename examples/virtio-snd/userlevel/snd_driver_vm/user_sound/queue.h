#pragma once
#include "sddf_snd_shared_ringbuffer.h"
#include <stdbool.h>

typedef struct pcm_queue pcm_queue_t;

pcm_queue_t *pcm_queue_create(int initial_capacity);

void pcm_queue_enqueue(pcm_queue_t *queue, const sddf_snd_pcm_data_t *pcm);

sddf_snd_pcm_data_t *pcm_queue_front(pcm_queue_t *queue);

bool pcm_queue_dequeue(pcm_queue_t *queue);

void pcm_queue_clear(pcm_queue_t *queue);

int pcm_queue_size(pcm_queue_t *queue);

bool pcm_queue_empty(pcm_queue_t *queue);
