#include "buffer_queue.h"

static int min(int a, int b)
{
    return a <= b ? a : b;
}

void buffer_queue_create(buffer_queue_t *queue, buffer_t *buffers, int capacity)
{
    queue->head = 0;
    queue->size = 0;
    queue->capacity = capacity;
    queue->buffers = buffers;
}

bool buffer_queue_enqueue(buffer_queue_t *queue, uintptr_t addr, unsigned int len)
{
    if (queue->size == queue->capacity) {
        return false;
    }

    int tail = (queue->head + queue->size) % queue->capacity;
    queue->buffers[tail].addr = addr;
    queue->buffers[tail].len = len;
    queue->size++;

    return true;
}

bool buffer_queue_dequeue(buffer_queue_t *queue, buffer_t *out)
{
    if (queue->size == 0) {
        return false;
    }

    *out = queue->buffers[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;
    return true;
}

void buffer_queue_clear(buffer_queue_t *queue)
{
    queue->size = 0;
    queue->head = 0;
}

int buffer_queue_size(buffer_queue_t *queue)
{
    return queue->size;
}

bool buffer_queue_empty(buffer_queue_t *queue)
{
    return queue->size == 0;
}
