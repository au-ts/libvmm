#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef struct buffer {
    uintptr_t addr;
    unsigned int len;
} buffer_t;

typedef struct buffer_queue {
    int head;
    int size;
    int capacity;
    buffer_t *buffers;
} buffer_queue_t;

void buffer_queue_create(buffer_queue_t *queue, buffer_t *buffers, int capacity);

bool buffer_queue_enqueue(buffer_queue_t *queue, uintptr_t addr, unsigned int len);

bool buffer_queue_dequeue(buffer_queue_t *queue, buffer_t *out);

void buffer_queue_clear(buffer_queue_t *queue);

int buffer_queue_size(buffer_queue_t *queue);

bool buffer_queue_empty(buffer_queue_t *queue);
