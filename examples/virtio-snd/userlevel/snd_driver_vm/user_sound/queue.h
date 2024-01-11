#pragma once
#include "sddf_snd_shared_ringbuffer.h"
#include <stdbool.h>

typedef struct cmd_queue cmd_queue_t;

cmd_queue_t *cmd_queue_create(int initial_capacity);

void cmd_queue_enqueue(cmd_queue_t *queue, const sddf_snd_command_t *cmd);

sddf_snd_command_t *cmd_queue_front(cmd_queue_t *queue);

bool cmd_queue_dequeue(cmd_queue_t *queue);

void cmd_queue_clear(cmd_queue_t *queue);

int cmd_queue_size(cmd_queue_t *queue);

bool cmd_queue_empty(cmd_queue_t *queue);
