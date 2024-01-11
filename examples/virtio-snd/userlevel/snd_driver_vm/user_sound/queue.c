#include "queue.h"
#include <stdlib.h>
#include <assert.h>

struct cmd_queue
{
    int head;
    int size;
    int capacity;
    sddf_snd_command_t *commands;
};

static int min(int a, int b)
{
    return a <= b ? a : b;
}

cmd_queue_t *cmd_queue_create(int initial_capacity)
{
    sddf_snd_command_t *commands = calloc(initial_capacity, sizeof(sddf_snd_command_t));
    if (commands == NULL) {
        return NULL;
    }

    cmd_queue_t *queue = malloc(sizeof(cmd_queue_t));
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

static void cmd_queue_resize(cmd_queue_t *queue)
{
    int tail = (queue->head + queue->size) % queue->capacity;

    queue->capacity *= 2;
    queue->commands = realloc(queue->commands, sizeof(sddf_snd_command_t) * queue->capacity);
    assert(queue->commands);

    if (queue->head > tail) {
        for (int i = 0; i < tail; i++) {
            queue->commands[i + queue->size] = queue->commands[i];
        }
    }
}

void cmd_queue_enqueue(cmd_queue_t *queue, const sddf_snd_command_t *cmd)
{
    if (queue->size == queue->capacity) {
        cmd_queue_resize(queue);
    }

    int tail = (queue->head + queue->size) % queue->capacity;
    queue->commands[tail] = *cmd;
    queue->size++;
}

void cmd_queue_clear(cmd_queue_t *queue)
{
    queue->size = 0;
    queue->head = 0;
}

sddf_snd_command_t *cmd_queue_front(cmd_queue_t *queue)
{
    if (queue->size == 0) {
        return false;
    }
    return &queue->commands[queue->head];
}

bool cmd_queue_dequeue(cmd_queue_t *queue)
{
    if (queue->size == 0) {
        return false;
    }

    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;
    return true;
}

int cmd_queue_size(cmd_queue_t *queue)
{
    return queue->size;
}

bool cmd_queue_empty(cmd_queue_t *queue)
{
    return queue->size == 0;
}
