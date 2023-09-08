/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * Vswitch implementation of the virtio_net transport translate layer
 */

#pragma once

#include <sel4cp.h>
#include <stdint.h>

#include "virtio_mmio.h"
#include "virtio_net_emul.h"
#include "virtio_net_interface.h"

#define VSWITCH_CONN_CH_1  2

typedef struct virtio_net_tt_vswitch {
    virtio_net_tt_interface_t *tt_interface;
    virtio_net_emul_interface_t *emul_interface;
    uintptr_t tx_avail;
    uintptr_t tx_used;
    uintptr_t tx_shared_dma_vaddr;
    uintptr_t rx_avail;
    uintptr_t rx_used;
    uintptr_t rx_shared_dma_vaddr;
    // Could need other bookkeeping fields here in the future
} virtio_net_tt_vswitch_t;

virtio_net_tt_vswitch_t *get_virtio_net_tt_vswitch(void);

// @ericc: arguments are sDDF ring buffers, an avail, used and shared dma region for both TX and RX
void virtio_net_tt_vswitch_init(uintptr_t tx_avail, uintptr_t tx_used, uintptr_t tx_shared_dma_vaddr,
                                uintptr_t rx_avail, uintptr_t rx_used, uintptr_t rx_shared_dma_vaddr);

