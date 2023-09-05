/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <sel4cp.h>

#define VSWITCH_CONN_CH_1  2

// @jade: this struct def should not be here. But we don't have a proper ethernet driver header file
// that contains this
typedef struct eth_driver_handler {
    void (*get_mac)(uint8_t *retval);
    int (*tx)(void *buf, uint32_t size);
    int (*rx)(sel4cp_channel ch);
} eth_driver_handler_t;

typedef int (*callback_fn_t)(void *buf, uint32_t size);

int vswitch_rx(sel4cp_channel channel);

eth_driver_handler_t *vswitch_init(callback_fn_t cb);
