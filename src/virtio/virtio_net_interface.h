/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * This file describes the functions that the emul layer and transport translate layer expose to each other 
 */

// @ericc: I expect this file is subject to constant refactors as we support more virtio_net features.

#pragma once

#include <sel4cp.h>

/************************ Implemented by TT Layer ************************/

/**
 * Gets the MAC address of the device.
 *
 * @param retval           Pointer to the buffer to store the MAC address
 *
 */
typedef void (*tt_get_mac)(uint8_t *retval);

/**
 * Handles transmitting a packet from the TX vqueue.
 *
 * @param buf           Buffer containing the packet
 * @param size          Size of the packet
 *
 * @return              0 on success
 */
typedef int (*tt_tx)(void *buf, uint32_t size);

/**
 * Handles receiving a packet from the source VM.
 *
 * @param ch            sel4cp channel number for notification
 *                      from the source VM
 *
 * @return              0 on success
 */
typedef int (*tt_rx)(sel4cp_channel ch);

typedef struct virtio_net_tt_interface {
    tt_get_mac get_mac;
    tt_tx tx;
    tt_rx rx;
} virtio_net_tt_interface_t;

virtio_net_tt_interface_t *get_virtio_net_tt_interface(void);

/************************ Implemented by Emul Layer ************************/

/**
 * Handles receiving a packet directed to the RX vqueue.
 *
 * @param buf           Buffer containing the packet
 * @param size          Size of the packet
 *
 * @return              0 on success
 */
typedef int (*emul_rx)(void *buf, uint32_t size);

typedef struct virtio_net_emul_interface {
    emul_rx rx;
} virtio_net_emul_interface_t;

virtio_net_emul_interface_t *get_virtio_net_emul_interface(void);
