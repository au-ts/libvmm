/*
 * Copyright 2022, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stddef.h>
#include <microkit.h>

#define UART_REG(x) ((volatile uint32_t *)(UART_BASE + (x)))
#define UART_BASE 0x5000000 //0x30890000 in hardware on imx8mm.
#define STAT 0x98
#define TRANSMIT 0x40
#define STAT_TDRE (1 << 14)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#ifdef __GNUC__
#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x)   (!!(x))
#define unlikely(x) (!!(x))
#endif

static void
putC(uint8_t ch)
{
    microkit_dbg_putc(ch);
}

static void
print(const char *s)
{
    microkit_dbg_puts(s);
}

static char
hexchar(unsigned int v)
{
    return v < 10 ? '0' + v : ('a' - 10) + v;
}

static void
puthex64(uint64_t val)
{
    char buffer[16 + 3];
    buffer[0] = '0';
    buffer[1] = 'x';
    buffer[16 + 3 - 1] = 0;
    for (unsigned i = 16 + 1; i > 1; i--) {
        buffer[i] = hexchar(val & 0xf);
        val >>= 4;
    }
    microkit_dbg_puts(buffer);
}

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
    print(&tmp[i]);
}

static void _assert_fail(
    const char  *assertion,
    const char  *file,
    unsigned int line,
    const char  *function)
{
    print("Failed assertion '");
    print(assertion);
    print("' at ");
    print(file);
    print(":");
    put8(line);
    print(" in function ");
    print(function);
    print("\n");
    while (1) {}
}

#ifdef NO_ASSERT

#define assert(expr)

#else

#define assert(expr) \
    do { \
        if (!(expr)) { \
            _assert_fail(#expr, __FILE__, __LINE__, __FUNCTION__); \
        } \
    } while(0)

#endif
