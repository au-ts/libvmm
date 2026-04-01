/*
 * Copyright 2021, Breakaway Consulting Pty. Ltd.
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <microkit.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <libvmm/util/printf.h>

#define SEL4_USER_CONTEXT_SIZE (sizeof(seL4_UserContext) / sizeof(seL4_Word))

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#endif

#define CTZ(x) __builtin_ctz(x)

#if __STDC_VERSION__ >= 201112L && !defined(__cplusplus)
#define static_assert _Static_assert
#endif

#define LOG_VMM(...) do{ printf("%s|INFO: ", microkit_name); printf(__VA_ARGS__); }while(0)
#define LOG_VMM_ERR(...) do{ printf("%s|ERROR (%s:%lu): ", microkit_name, __FUNCTION__, __LINE__); printf(__VA_ARGS__); }while(0)

static void assert_fail(const char *assertion, const char *file, unsigned int line, const char *function)
{
    printf("Failed assertion '%s' at %s:%u in function %s\n", assertion, file, line, function);
    __builtin_trap();
}

#define BIT_LOW(n)  (1ul<<(n))
#define BIT_HIGH(n) (1ul<<(n - 32 ))

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* Convenience function to print memory region in hex */
void print_mem_hex(uintptr_t addr, size_t size);

#ifndef assert
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
#endif

/* Returns true if all set bits in `baseline` are also set in `actual` */
bool check_baseline_bits(uint64_t baseline, uint64_t actual);

/* Print all bit indexes that are set in `baseline` but not set in `actual` */
void print_missing_baseline_bits(uint64_t baseline, uint64_t actual);

/* Returns true if two [x..y) ranges overlap. */
bool ranges_overlap(uint64_t left_start, uint64_t left_size, uint64_t right_start, uint64_t right_size);

#if defined(CONFIG_ARCH_X86_64)
static inline uint64_t rdtsc(void)
{
    uint32_t lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static inline void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *a, uint32_t *b, uint32_t *c, uint32_t *d)
{
    __asm__ __volatile__("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "a"(leaf), "c"(subleaf));
}
#endif
