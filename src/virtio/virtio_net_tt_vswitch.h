/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <sel4cp.h>
#include <stdint.h>

#include "virtio_mmio.h"
#include "virtio_net_emul.h"
#include "virtio_net_interface.h"

#define VSWITCH_CONN_CH_1  2

// @ericc: This struct is maybe premature generalisation?
// The idea is that the next layer may need this struct for its implementation
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

void virtio_net_tt_vswitch_init(uintptr_t tx_avail, uintptr_t tx_used, uintptr_t tx_shared_dma_vaddr,
                                uintptr_t rx_avail, uintptr_t rx_used, uintptr_t rx_shared_dma_vaddr);

virtio_net_tt_vswitch_t *get_virtio_net_tt_vswitch(void);
