/*
 * Copyright 2022, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sddf/network/constants.h>
#include <sddf/util/fence.h>
#include <sddf/util/util.h>

/**
 * Each function cannot be concurrently called by more than one thread on the same input argument.
 * Different functions can be called concurrently, even though they are accessing the same net_queue_t.
*/

typedef struct net_buff_desc {
    /* offset of buffer within buffer memory region or io address of buffer */
    uint64_t io_or_offset;
    /* length of data inside buffer */
    uint16_t len;
} net_buff_desc_t;

typedef struct net_queue {
    /* index to insert at */
    uint16_t tail;
    /* index to remove from */
    uint16_t head;
    /* flag to indicate whether consumer requires signalling */
    uint32_t consumer_signalled;
    /* buffer descripter array */
    net_buff_desc_t buffers[];
} net_queue_t;

typedef struct net_queue_handle {
    /* available buffers */
    net_queue_t *free;
    /* filled buffers */
    net_queue_t *active;
    /* size of the queues */
    uint32_t capacity;
} net_queue_handle_t;

#define CONFIG_ENABLE_SMP_SUPPORT 1

/**
 * Get the number of buffers enqueued into a queue.
 *
 * @param queue queue handle for the queue to get the size of.
 *
 * @return number of buffers enqueued into a queue.
 */
static inline uint16_t net_queue_size(net_queue_t *queue)
{
#if CONFIG_ENABLE_SMP_SUPPORT
    uint16_t tail = __atomic_load_n(&queue->tail, __ATOMIC_RELAXED);
    uint16_t head = __atomic_load_n(&queue->head, __ATOMIC_RELAXED);
    uint16_t ret = tail - head;

    // TODO: this is expensive
    // alternatively, uses acquire load on tail
    // even better, decide whether the function should have acquire semantics externally,
    // and uses plain relaxed loads if the acquire semantics is not required
    THREAD_MEMORY_ACQUIRE();

    return ret;
#else
    return queue->tail - queue->head;
#endif
}

/**
 * Check if the free queue is empty.
 *
 * @param queue queue handle for the free queue to check.
 *
 * @return true indicates the queue is empty, false otherwise.
 */
static inline bool net_queue_empty_free(net_queue_handle_t *queue)
{
#if CONFIG_ENABLE_SMP_SUPPORT
    uint16_t tail = __atomic_load_n(&queue->free->tail, __ATOMIC_ACQUIRE);
    uint16_t head = __atomic_load_n(&queue->free->head, __ATOMIC_RELAXED);
    bool ret = tail - head == 0;

    return ret;
#else
    return queue->free->tail - queue->free->head == 0;
#endif
}

/**
 * Check if the active queue is empty.
 *
 * @param queue queue handle for the active queue to check.
 *
 * @return true indicates the queue is empty, false otherwise.
 */
static inline bool net_queue_empty_active(net_queue_handle_t *queue)
{
#if CONFIG_ENABLE_SMP_SUPPORT
    uint16_t tail = __atomic_load_n(&queue->active->tail, __ATOMIC_ACQUIRE);
    uint16_t head = __atomic_load_n(&queue->active->head, __ATOMIC_RELAXED);
    bool ret = tail - head == 0;
    return ret;
#else
    return queue->active->tail - queue->active->head == 0;
#endif
}

/**
 * Check if the free queue is full.
 *
 * @param queue queue handle t for the free queue to check.
 *
 * @return true indicates the queue is full, false otherwise.
 */
static inline bool net_queue_full_free(net_queue_handle_t *queue)
{
#if CONFIG_ENABLE_SMP_SUPPORT
    uint16_t head = __atomic_load_n(&queue->free->head, __ATOMIC_ACQUIRE);
    uint16_t tail = __atomic_load_n(&queue->free->tail, __ATOMIC_RELAXED);
    bool ret = tail + 1 - head == queue->capacity;
    return ret;
#else
    return queue->free->tail + 1 - queue->free->head == queue->capacity;
#endif
}

/**
 * Check if the active queue is full.
 *
 * @param queue queue handle for the active queue to check.
 *
 * @return true indicates the queue is full, false otherwise.
 */
static inline bool net_queue_full_active(net_queue_handle_t *queue)
{
#if CONFIG_ENABLE_SMP_SUPPORT
    uint16_t head = __atomic_load_n(&queue->active->head, __ATOMIC_ACQUIRE);
    uint16_t tail = __atomic_load_n(&queue->active->tail, __ATOMIC_RELAXED);
    bool ret = tail + 1 - head == queue->capacity;
    return ret;
#else
    return queue->active->tail + 1 - queue->active->head == queue->capacity;
#endif
}

/**
 * Enqueue an element into a free queue.
 *
 * @param queue queue to enqueue into.
 * @param buffer buffer descriptor for buffer to be enqueued.
 *
 * @return -1 when queue is full, 0 on success.
 */
static inline int net_enqueue_free(net_queue_handle_t *queue, net_buff_desc_t buffer)
{
    if (net_queue_full_free(queue)) {
        return -1;
    }

#if CONFIG_ENABLE_SMP_SUPPORT
    // net_queue_full_free performs acquire read on the member "tail"
    // by coherence-RR, the second read is guaranteed to see the same value of the release write
    uint16_t tail = __atomic_load_n(&queue->free->tail, __ATOMIC_RELAXED);
    queue->free->buffers[tail % queue->capacity] = buffer;
    __atomic_store_n(&queue->free->tail, tail + 1, __ATOMIC_RELEASE);
#else
    queue->free->buffers[queue->free->tail % queue->capacity] = buffer;
    COMPILER_MEMORY_RELEASE();
    queue->free->tail++;
#endif

    return 0;
}

/**
 * Enqueue an element into an active queue.
 *
 * @param queue queue to enqueue into.
 * @param buffer buffer descriptor for buffer to be enqueued.
 *
 * @return -1 when queue is full, 0 on success.
 */
static inline int net_enqueue_active(net_queue_handle_t *queue, net_buff_desc_t buffer)
{
    if (net_queue_full_active(queue)) {
        return -1;
    }

#if CONFIG_ENABLE_SMP_SUPPORT
    uint16_t tail = __atomic_load_n(&queue->active->tail, __ATOMIC_RELAXED);
    queue->active->buffers[tail % queue->capacity] = buffer;
    __atomic_store_n(&queue->active->tail, tail + 1, __ATOMIC_RELEASE);
#else
    queue->active->buffers[queue->active->tail % queue->capacity] = buffer;
    COMPILER_MEMORY_RELEASE();
    queue->active->tail++;
#endif

    return 0;
}

/**
 * Dequeue an element from the free queue.
 *
 * @param queue queue handle to dequeue from.
 * @param buffer pointer to buffer descriptor for buffer to be dequeued.
 *
 * @return -1 when queue is empty, 0 on success.
 */
static inline int net_dequeue_free(net_queue_handle_t *queue, net_buff_desc_t *buffer)
{
    if (net_queue_empty_free(queue)) {
        return -1;
    }

#if CONFIG_ENABLE_SMP_SUPPORT
    uint16_t head = __atomic_load_n(&queue->free->head, __ATOMIC_RELAXED);
    *buffer = queue->free->buffers[head % queue->capacity];
    __atomic_store_n(&queue->free->head, head + 1, __ATOMIC_RELEASE);
#else
    *buffer = queue->free->buffers[queue->free->head % queue->capacity];
    COMPILER_MEMORY_RELEASE();
    queue->free->head++;
#endif

    return 0;
}

/**
 * Dequeue an element from the active queue.
 *
 * @param queue queue handle to dequeue from.
 * @param buffer pointer to buffer descriptor for buffer to be dequeued.
 *
 * @return -1 when queue is empty, 0 on success.
 */
static inline int net_dequeue_active(net_queue_handle_t *queue, net_buff_desc_t *buffer)
{
    if (net_queue_empty_active(queue)) {
        return -1;
    }

    
#if CONFIG_ENABLE_SMP_SUPPORT
    uint16_t head = __atomic_load_n(&queue->active->head, __ATOMIC_RELAXED);
    *buffer = queue->active->buffers[head % queue->capacity];
    __atomic_store_n(&queue->active->head, head + 1, __ATOMIC_RELEASE);
#else
    *buffer = queue->active->buffers[queue->active->head % queue->capacity];
    COMPILER_MEMORY_RELEASE();
    queue->active->head++;
#endif

    return 0;
}

/**
 * Initialise the shared queue.
 *
 * @param queue queue handle to use.
 * @param free pointer to free queue in shared memory.
 * @param active pointer to active queue in shared memory.
 * @param size size of the free and active queues.
 */
static inline void net_queue_init(net_queue_handle_t *queue, net_queue_t *free, net_queue_t *active, uint32_t size)
{
    queue->free = free;
    queue->active = active;
    queue->capacity = size;
}

/**
 * Initialise the free queue by filling with all free buffers.
 *
 * @param queue queue handle to use.
 * @param base_addr start of the memory region the offsets are applied to (only used between virt and driver)
 */
static inline void net_buffers_init(net_queue_handle_t *queue, uintptr_t base_addr)
{
    for (uint32_t i = 0; i < queue->capacity - 1; i++) {
        net_buff_desc_t buffer = {(NET_BUFFER_SIZE * i) + base_addr, 0};
        int err = net_enqueue_free(queue, buffer);
        assert(!err);
    }
}

/**
 * Indicate to producer of the free queue that consumer requires signalling.
 *
 * @param queue queue handle of free queue that requires signalling upon enqueuing.
 */
static inline void net_request_signal_free(net_queue_handle_t *queue)
{
#if CONFIG_ENABLE_SMP_SUPPORT
    __atomic_store_n(&queue->free->consumer_signalled, 0, __ATOMIC_RELEASE);
#else
    queue->free->consumer_signalled = 0;
#endif
}

/**
 * Indicate to producer of the active queue that consumer requires signalling.
 *
 * @param queue queue handle of active queue that requires signalling upon enqueuing.
 */
static inline void net_request_signal_active(net_queue_handle_t *queue)
{
#if CONFIG_ENABLE_SMP_SUPPORT
    __atomic_store_n(&queue->active->consumer_signalled, 0, __ATOMIC_RELEASE);
#else
    queue->active->consumer_signalled = 0;
#endif
}

/**
 * Indicate to producer of the free queue that consumer has been signalled.
 *
 * @param queue queue handle of the free queue that has been signalled.
 */
static inline void net_cancel_signal_free(net_queue_handle_t *queue)
{
#if CONFIG_ENABLE_SMP_SUPPORT
    __atomic_store_n(&queue->free->consumer_signalled, 1, __ATOMIC_RELEASE);
#else
    queue->free->consumer_signalled = 1;
#endif
}

/**
 * Indicate to producer of the active queue that consumer has been signalled.
 *
 * @param queue queue handle of the active queue that has been signalled.
 */
static inline void net_cancel_signal_active(net_queue_handle_t *queue)
{
#if CONFIG_ENABLE_SMP_SUPPORT
    __atomic_store_n(&queue->active->consumer_signalled, 1, __ATOMIC_RELEASE);
#else
    queue->active->consumer_signalled = 1;
#endif
}

/**
 * Consumer of the free queue requires signalling.
 *
 * @param queue queue handle of the free queue to check.
 */
static inline bool net_require_signal_free(net_queue_handle_t *queue)
{
#if CONFIG_ENABLE_SMP_SUPPORT
    return !__atomic_load_n(&queue->free->consumer_signalled, __ATOMIC_ACQUIRE);
#else
    return !queue->free->consumer_signalled;
#endif
}

/**
 * Consumer of the active queue requires signalling.
 *
 * @param queue queue handle of the active queue to check.
 */
static inline bool net_require_signal_active(net_queue_handle_t *queue)
{
#if CONFIG_ENABLE_SMP_SUPPORT
    return !__atomic_load_n(&queue->active->consumer_signalled, __ATOMIC_ACQUIRE);
#else
    return !queue->active->consumer_signalled;
#endif
}
