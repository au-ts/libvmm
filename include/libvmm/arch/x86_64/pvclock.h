/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdbool.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>

#define MSR_KVM_WALL_CLOCK_NEW 0x4b564d00
#define MSR_KVM_SYSTEM_TIME_NEW 0x4b564d01

struct pvclock_wall_clock {
    uint32_t version;
    uint32_t sec;
    uint32_t nsec;
} __attribute__((__packed__));

struct pvclock_vcpu_time_info {
    uint32_t version;
    uint32_t pad0;
    uint64_t tsc_timestamp;
    uint64_t system_time;
    uint32_t tsc_to_system_mul;
    int8_t tsc_shift; // note that this is signed
    uint8_t flags;
    uint8_t pad[2];
} __attribute__((__packed__)); /* 32 bytes */

bool pvclock_write_fault_handle(uint64_t msr_addr, uint64_t msr_value);