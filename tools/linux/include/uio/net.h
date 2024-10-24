/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>

#include <ethernet_config.h>

typedef struct {
    /* Any info that the VMM wants to give us go in here */
    uint64_t rx_paddr;
    uint64_t tx_paddrs[NUM_NETWORK_CLIENTS];
} vmm_net_info_t;
