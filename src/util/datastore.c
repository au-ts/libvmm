#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "datastore.h"

// @ericc: When we have libc implementation replace this
static void *memcpy(void *restrict dest, const void *restrict src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    for (; n; n--) *d++ = *s++;
    return dest;
}

int datastore_alloc(datastore_t *ds, void *data, uint64_t *id)
{
    if (datastore_full(ds)) {
        return -1;
    }

    // Store data into head of storage
    void *dest = (uint8_t *)ds->storage + ds->head * ds->data_type_size;
    memcpy(dest, data, ds->data_type_size);

    // Give the data ID to the caller
    *id = ds->head;

    // Indicate storage slot is used
    ds->used[ds->head] = true;
    
    // Update head to next free storage slot
    ds->head = ds->nextfree[ds->head];

    // Update number of free storage slots
    // @ericc: Overflow?
    ds->num_free--;
    
    return 0;
}

int datastore_retrieve(datastore_t *ds, uint64_t id, void *data)
{
    if (id >= ds->size || !ds->used[id]) {
        // Invalid ID
        return -1;
    }

    if (datastore_full(ds)) {
        // Head and tail points to stale index, so restore it
        ds->head = id;
        ds->tail = id;
    } else {
        ds->nextfree[ds->tail] = id;
        ds->tail = id;
    }

    // Indicate storage slot is free
    ds->used[id] = false;

    // @ericc: Overflow?
    ds->num_free++;

    memcpy(data, (uint8_t *)ds->storage + id * ds->data_type_size, ds->data_type_size);

    return 0;
}

void datastore_init(datastore_t *ds, void *storage, size_t data_type_size, uint64_t *nextfree, bool *used, uint64_t size)
{
    ds->storage = storage;
    ds->data_type_size = data_type_size;
    ds->nextfree = nextfree;
    ds->used = used;
    ds->size = size;
    ds->head = 0;
    ds->tail = size - 1;
    ds->num_free = size;

    for (uint64_t i = 0; i < size - 1; i++) {
        ds->nextfree[i] = i + 1;
    }
    ds->nextfree[size - 1] = 0;

    for (uint64_t i = 0; i < size; i++) {
        ds->used[i] = false;
    }
}