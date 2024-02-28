/*
 * Copyright 2021, Breakaway Consulting Pty. Ltd.
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <microkit.h>
#include "printf.h"
#include "bitarray.h"

// @ivanv: these are here for convience, should not be here though
#define GUEST_VCPU_ID 0
#define GUEST_NUM_VCPUS 1

// @ivanv: if we keep using this, make sure that we have a static assert
// that sizeof seL4_UserContext is 0x24
// Note that this is AArch64 specific
#if defined(CONFIG_ARCH_AARCH64)
    #define SEL4_USER_CONTEXT_SIZE 0x24
#endif

#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))

#define CTZ(x) __builtin_ctz(x)

#if __STDC_VERSION__ >= 201112L && !defined(__cplusplus)
#define static_assert _Static_assert
#endif

#define LOG_VMM(...) do{ printf("%s|INFO: ", microkit_name); printf(__VA_ARGS__); }while(0)
#define LOG_VMM_ERR(...) do{ printf("%s|ERROR: ", microkit_name); printf(__VA_ARGS__); }while(0)

static void *memcpy(void *restrict dest, const void *restrict src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    for (; n; n--) *d++ = *s++;
    return dest;
}

static void *memset(void *dest, int c, size_t n)
{
    unsigned char *s = dest;
    for (; n; n--, s++) *s = c;
    return dest;
}

static void assert_fail(
    const char  *assertion,
    const char  *file,
    unsigned int line,
    const char  *function)
{
    printf("Failed assertion '%s' at %s:%u in function %s\n", assertion, file, line, function);
    while (1) {}
}

#define BIT_LOW(n)  (1ul<<(n))
#define BIT_HIGH(n) (1ul<<(n - 32 ))

/* Convenience function to print a bit array */
void print_bitarray(bitarray_t* bitarr);
/* Convenience function to print word in bits */
void print_binary(word_t word);

#ifndef CONFIG_DEBUG_BUILD

#define _unused(x) ((void)(x))
#define assert(expr) _unused(expr)

#else

#define assert(expr) \
    do { \
        if (!(expr)) { \
            assert_fail(#expr, __FILE__, __LINE__, __FUNCTION__); \
        } \
    } while(0)

#endif
