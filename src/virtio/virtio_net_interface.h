/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <sel4cp.h>

// @ericc: If you want to change how emul and tt layer communicate with each other,
// modify this file.

// Implemented by tt layer (used by emul layer)
typedef struct virtio_net_tt_interface {
    void (*get_mac)(uint8_t *retval);
    int (*tx)(void *buf, uint32_t size);
    int (*rx)(sel4cp_channel ch);
} virtio_net_tt_interface_t;

virtio_net_tt_interface_t *get_virtio_net_tt_interface(void);

// Implemented by emul layer (used by tt layer)
typedef struct virtio_net_emul_interface {
    // @ericc: Maybe this should take in IRQ number? Not sure 
    // whether we want other layers to know which IRQ to inject to.
    // For now it doesn't.
    bool (*send_interrupt)();
    int (*rx)(void *buf, uint32_t size);
} virtio_net_emul_interface_t;

virtio_net_emul_interface_t *get_virtio_net_emul_interface(void);
