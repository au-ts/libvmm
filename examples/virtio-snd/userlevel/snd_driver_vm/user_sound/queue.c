#include "queue.h"
#include <stdlib.h>
#include <assert.h>

struct pcm_queue
{
    int head;
    int size;
    int capacity;
    sddf_snd_pcm_data_t *commands;
};

static int min(int a, int b)
{
    return a <= b ? a : b;
}

pcm_queue_t *pcm_queue_create(int initial_capacity)
{
    sddf_snd_pcm_data_t *commands = calloc(initial_capacity, sizeof(sddf_snd_pcm_data_t));
    if (commands == NULL) {
        return NULL;
    }

    pcm_queue_t *queue = malloc(sizeof(pcm_queue_t));
    if (queue == NULL) {
        free(commands);
        return NULL;
    }

    queue->head = 0;
    queue->size = 0;
    queue->capacity = initial_capacity;
    queue->commands = commands;

    return queue;
}

static void pcm_queue_resize(pcm_queue_t *queue)
{
    int tail = (queue->head + queue->size) % queue->capacity;

    queue->capacity *= 2;
    queue->commands = realloc(queue->commands, sizeof(sddf_snd_pcm_data_t) * queue->capacity);
    assert(queue->commands);

    if (queue->head > tail) {
        for (int i = 0; i < tail; i++) {
            queue->commands[i + queue->size] = queue->commands[i];
        }
    }
}

void pcm_queue_enqueue(pcm_queue_t *queue, const sddf_snd_pcm_data_t *pcm)
{
    if (queue->size == queue->capacity) {
        pcm_queue_resize(queue);
    }

    int tail = (queue->head + queue->size) % queue->capacity;
    queue->commands[tail] = *pcm;
    queue->size++;
}

void pcm_queue_clear(pcm_queue_t *queue)
{
    queue->size = 0;
    queue->head = 0;
}

sddf_snd_pcm_data_t *pcm_queue_front(pcm_queue_t *queue)
{
    if (queue->size == 0) {
        return false;
    }
    return &queue->commands[queue->head];
}

bool pcm_queue_dequeue(pcm_queue_t *queue)
{
    if (queue->size == 0) {
        return false;
    }

    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;
    return true;
}

int pcm_queue_size(pcm_queue_t *queue)
{
    return queue->size;
}

bool pcm_queue_empty(pcm_queue_t *queue)
{
    return queue->size == 0;
}
