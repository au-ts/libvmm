/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>

/* IMPORTANT: make sure these paths and names matches with DTS!! */

#define UIO_NET_DATA_PASSING_DEVICE "uio0"
#define UIO_NET_RX_FREE_DEVICE "uio1"
#define UIO_NET_TX_FREE_DEVICE "uio2"
#define UIO_NET_RX_ACTIVE_DEVICE "uio3"
#define UIO_NET_TX_ACTIVE_DEVICE "uio4"
/* This is how the guest can signal back to VMM, the physical address of these
   UIO regions are unmapped, so a fault is generated */
#define UIO_NET_RX_FAULT_DEVICE "uio5"
#define UIO_NET_TX_FAULT_DEVICE "uio6"
/* This is how the VMM notify us of notifications from virt TX and RX.
   Once IRQ is enabled by writing to a UIO FD from these, a read on the same
   FD will block until the VMM does a virq_inject on the associated IRQ number
   in device tree. */
#define UIO_NET_RX_IRQ_DEVICE "uio7"
#define UIO_NET_TX_IRQ_DEVICE "uio8"
#define UIO_NET_RX_DATA_DEVICE "uio9"
#define UIO_NET_TX_DATA_DEVICE "uio10"

#define UIO_NET_DATA_PASSING_NAME "data_passing"
#define UIO_NET_RX_FREE_NAME "rx_free"
#define UIO_NET_TX_FREE_NAME "tx_free"
#define UIO_NET_RX_ACTIVE_NAME "rx_active"
#define UIO_NET_TX_ACTIVE_NAME "tx_active"
#define UIO_NET_RX_FAULT_NAME "rx_fault"
#define UIO_NET_TX_FAULT_NAME "tx_fault"
#define UIO_NET_RX_IRQ_NAME "rx_irq"
#define UIO_NET_TX_IRQ_NAME "tx_irq"
#define UIO_NET_RX_DATA_NAME "rx_data"
#define UIO_NET_TX_DATA_NAME "tx_data"

typedef struct net_drv_vm_data_passing {
    net_driver_config_t net_config;
    uint64_t rx_data_paddr;
    int num_tx_clients;
    uint64_t tx_data_paddrs[SDDF_NET_MAX_CLIENTS];
} net_drv_vm_data_passing_t;