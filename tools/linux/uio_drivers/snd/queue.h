/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>

typedef struct queue queue_t;

queue_t *queue_create(int item_size, int initial_capacity);

void queue_enqueue(queue_t *queue, const void *item);

void *queue_front(queue_t *queue);

bool queue_dequeue(queue_t *queue);

void queue_clear(queue_t *queue);

int queue_size(queue_t *queue);

bool queue_empty(queue_t *queue);
