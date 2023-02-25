/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <sel4cp.h>

#define VSWITCH_CONN_1_CH  2
#define VSWITCH_CONN_2_CH  3

// @jade: this struct def should not be here. But we don't have a proper ethernet driver header file
// that contains this
typedef struct eth_driver_handler {
    void (*get_mac)(uint8_t *retval);
    int (*tx)(void *buf, uint32_t size);
    int (*rx)(sel4cp_channel ch);
} eth_driver_handler_t;

// a description of a node
// typedef struct vswitch_node {
//     // ptrs to the shared buffer
//     uintptr_t rx_avail;
//     uintptr_t rx_used;
//     uintptr_t tx_avail;
//     uintptr_t tx_used;
//     uintptr_t rx_shared_dma_vaddr;
//     uintptr_t tx_shared_dma_vaddr;

//     sel4cp_channel channel;
//     uint8_t peer_mac[6];
// } vswitch_node_t;

typedef int (*callback_fn_t)(void *buf, uint32_t size);

int vswitch_rx(sel4cp_channel channel);

eth_driver_handler_t *vswitch_init(callback_fn_t cb);
