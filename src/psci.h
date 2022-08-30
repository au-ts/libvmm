/*
 * Copyright 2020, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: GPL-2.0-only
 */

#include <sel4cp.h>

/* Values in this file are taken from the:
 * ARM Power State Coordination Interface
 * Platform Design Document
 * Issue D
 */

/* PSCI return codes */
#define PSCI_SUCCESS 0
#define PSCI_NOT_SUPPORTED -1
#define PSCI_INVALID_PARAMETERS -2
#define PSCI_DENIED -3
#define PSCI_ALREADY_ON -4
#define PSCI_ON_PENDING -5
#define PSCI_INTERNAL_FAILURE -6
#define PSCI_NOT_PRESENT -7
#define PSCI_DISABLED -8
#define PSCI_INVALID_ADDRESS -9

// PSCI functoin IDs
typedef enum psci {
    PSCI_VERSION = 0x0,
    PSCI_CPU_SUSPEND = 0x1,
    PSCI_CPU_OFF = 0x2,
    PSCI_CPU_ON = 0x3,
    PSCI_AFFINTY_INFO = 0x4,
    PSCI_MIGRATE = 0x5,
    PSCI_MIGRATE_INFO_TYPE = 0x6,
    PSCI_MIGRATE_INFO_UP_CPU = 0x7,
    PSCI_SYSTEM_OFF = 0x8,
    PSCI_SYSTEM_RESET = 0x9,
    PSCI_FEATURES = 0xa,
    PSCI_CPU_FREEZE = 0xb,
    PSCI_CPU_DEFAULT_SUSPEND = 0xc,
    PSCI_NODE_HW_STATE = 0xd,
    PSCI_SYSTEM_SUSPEND = 0xe,
    PSCI_SET_SUSPEND_MODE = 0xf,
    PSCI_STAT_RESIDENCY = 0x10,
    PSCI_STAT_COUNT = 0x11,
    PSCI_SYSTEM_RESET2 = 0x12,
    PSCI_MEM_PROTECT = 0x13,
    PSCI_MEM_PROTECT_CHECK_RANGE = 0x14,
    PSCI_MAX = 0x1f
} psci_id_t;

bool handle_psci(uint64_t vcpu_id, seL4_UserContext *regs,  uint64_t fn_number);
