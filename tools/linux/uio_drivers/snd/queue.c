#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

struct queue
{
    int head;
    int len;
    int capacity;
    int item_size;
    void *data;
};

static int min(int a, int b)
{
    return a <= b ? a : b;
}

queue_t *queue_create(int item_size, int initial_capacity)
{
    void *data = calloc(initial_capacity, item_size);
    if (data == NULL) {
        return NULL;
    }

    queue_t *queue = malloc(sizeof(queue_t));
    if (queue == NULL) {
        free(data);
        return NULL;
    }

    queue->head = 0;
    queue->len = 0;
    queue->capacity = initial_capacity;
    queue->item_size = item_size;
    queue->data = data;

    return queue;
}

static void pcm_queue_resize(queue_t *queue)
{
    int tail = (queue->head + queue->len) % queue->capacity;

    queue->capacity *= 2;
    queue->data = realloc(queue->data, queue->item_size * queue->capacity);
    assert(queue->data);

    memcpy(queue->data + queue->item_size * queue->len, queue->data, tail * queue->item_size);
}

void queue_enqueue(queue_t *queue, const void *item)
{
    if (queue->len == queue->capacity) {
        pcm_queue_resize(queue);
    }

    int tail = (queue->head + queue->len) % queue->capacity;
    memcpy(queue->data + (tail * queue->item_size), item, queue->item_size);
    queue->len++;
}

void queue_clear(queue_t *queue)
{
    queue->len = 0;
    queue->head = 0;
}

void *queue_front(queue_t *queue)
{
    if (queue->len == 0) {
        return NULL;
    }
    return queue->data + queue->head * queue->item_size;
}

bool queue_dequeue(queue_t *queue)
{
    if (queue->len == 0) {
        return false;
    }

    queue->head = (queue->head + 1) % queue->capacity;
    queue->len--;
    return true;
}

int queue_size(queue_t *queue)
{
    return queue->len;
}

bool queue_empty(queue_t *queue)
{
    return queue->len == 0;
}
