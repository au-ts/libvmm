#pragma once
#include <stdint.h>

typedef struct buffer {
    uintptr_t addr;
    unsigned int len;
} buffer_t;
