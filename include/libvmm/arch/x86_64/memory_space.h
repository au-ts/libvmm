/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <libvmm/util/util.h>

/* Documents referenced:
 * 1. Intel® 64 and IA-32 Architectures Software Developer’s Manual
 *    Combined Volumes: 1, 2A, 2B, 2C, 2D, 3A, 3B, 3C, 3D, and 4
 *    Order Number: 325462-080US June 2023
 * 2. https://en.wikipedia.org/wiki/High_Precision_Event_Timer
 */

/* Document layout of guest RAM */
#define LOW_RAM_START_GPA 0x0

#define ECAM_GPA 0xD0000000
/* 1 bus, 32 devices per bus, 8 functions per device, 4k config space per function */
#define ECAM_SIZE (1 * 32 * 8 * 4096)

#define IOAPIC_GPA 0xFEC00000
#define IOAPIC_SIZE 0x1000

/* This just matches the x86 CPU default physical address from
 * See bit 9 of [1] 'Table 1-20. More on Feature Information Returned in the EDX Register'.
 */
#define LAPIC_GPA 0xFEE00000
#define LAPIC_SIZE 0x1000

/* The HPET spec doesn't specify where the registers should be placed. So
 * we just use the conventional x86 PC placement. See note a. of [2] */
#define HPET_GPA 0xFED00000
#define HPET_SIZE 0x1000

#define HIGH_RAM_START_GPA 0x100000000
