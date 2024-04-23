#pragma once
#include <stdbool.h>
#include "util.h"

/**
 * Generic fixed-size circular queue
 */
typedef struct queue {
    int head;
    int len;
    int capacity;
    int item_size;
    void *data;
} queue_t;

static void queue_init(queue_t *q, int item_size, void *data, int capacity)
{
    q->head = 0;
    q->len = 0;
    q->capacity = capacity;
    q->item_size = item_size;
    q->data = data;
}

/** Enqueue a small item to the queue */
static bool queue_enqueue(queue_t *queue, const void *item)
{
    if (queue->len == queue->capacity) {
        return false;
    }

    int tail = (queue->head + queue->len) % queue->capacity;
    memcpy(queue->data + (tail * queue->item_size), item, queue->item_size);
    queue->len++;

    return true;
}

/** Enqueue a large item to the queue */
static void *queue_enqueue_raw(queue_t *queue)
{
    if (queue->len == queue->capacity) {
        return NULL;
    }

    int tail = (queue->head + queue->len) % queue->capacity;
    void *data = queue->data + (tail * queue->item_size);

    queue->len++;
    return data;
}

static inline void queue_clear(queue_t *queue)
{
    queue->len = 0;
    queue->head = 0;
}

/**
 * Get first element in queue.
 * This can be used with `queue_dequeue` to avoid copying if items are large.
 */
static void *queue_front(queue_t *queue)
{
    if (queue->len == 0) {
        return NULL;
    }
    return queue->data + queue->head * queue->item_size;
}

/** Remove first element from queue */
static bool queue_dequeue(queue_t *queue)
{
    if (queue->len == 0) {
        return false;
    }

    queue->head = (queue->head + 1) % queue->capacity;
    queue->len--;
    return true;
}

/** Get oldest element in queue and remove it */
static bool queue_dequeue_front(queue_t *queue, void *dest)
{
    if (queue->len == 0) {
        return false;
    }

    const void *src = queue->data + queue->head * queue->item_size;
    memcpy(dest, src, queue->item_size);

    queue->head = (queue->head + 1) % queue->capacity;
    queue->len--;
    return true;
}

/** Remove most recently added element from queue */
static bool queue_dequeue_back(queue_t *queue)
{
    if (queue->len == 0) {
        return false;
    }

    queue->len--;
    return true;
}

static inline int queue_size(queue_t *queue)
{
    return queue->len;
}

static inline bool queue_empty(queue_t *queue)
{
    return queue->len == 0;
}
