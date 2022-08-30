#pragma once

#include <stdint.h>
#include <sel4cp.h>
#include "util/printf.h"

// @ivanv: these are here for convience, should not be here though
#define VM_ID 1
#define VCPU_ID 0
// Note that this is AArch64 specific
#define SEL4_USER_CONTEXT_SIZE 0x24

void halt(int cond);
void _putchar(char character);

static char
decchar(unsigned int v) {
    return '0' + v;
}

static void
put8(uint8_t x)
{
    char tmp[4];
    unsigned i = 3;
    tmp[3] = 0;
    do {
        uint8_t c = x % 10;
        tmp[--i] = decchar(c);
        x /= 10;
    } while (x);
    sel4cp_dbg_puts(&tmp[i]);
}
