#pragma once
#include <stdint.h>
#include <stdbool.h>

#define FREELIST_INVALID ((uint32_t)-1)

typedef struct freelist {
    uint32_t head;
    uint32_t tail;
    uint32_t free_count;
    // Index is free message ID, maps to next free command ID
    uint32_t *freelist;
    uint32_t capacity;
} freelist_t;

static void freelist_init(freelist_t *list, uint32_t *freelist_data, uint32_t capacity)
{
    list->head = 0;
    list->tail = capacity - 1;
    list->free_count = capacity;

    list->freelist = freelist_data;
    list->capacity = capacity;

    for (uint32_t i = 0; i < capacity - 1; i++) {
        list->freelist[i] = i + 1;
    }
    list->freelist[capacity - 1] = FREELIST_INVALID;
}

/** Check if the command store is full. */
static inline bool freelist_full(freelist_t *freelist)
{
    return freelist->free_count == 0;
}

/**
 * Allocate a command store slot for a new virtio command.
 * 
 * @param freelist the freelist
 * @param id pointer to command ID allocated
 * @return 0 on success, FREELIST_INVALID on failure
 */
static uint32_t freelist_allocate(freelist_t *freelist)
{
    if (freelist_full(freelist)) {
        return FREELIST_INVALID;
    }

    uint32_t id = freelist->head;

    // Update head to next free command store slot
    freelist->head = freelist->freelist[freelist->head];

    // Update number of free command store slots
    freelist->free_count--;
    
    return id;
}

/** Return an id to the freelist */
static void freelist_return(freelist_t *freelist, uint32_t id)
{
    if (freelist->free_count == 0) {
        // Head points to stale index, so restore it
        freelist->head = id;
    }

    freelist->freelist[freelist->tail] = id;
    freelist->tail = id;

    freelist->free_count++;
}
