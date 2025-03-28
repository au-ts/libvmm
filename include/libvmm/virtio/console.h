/*
 * This header is BSD licensed so
 * anyone can use the definitions to implement compatible drivers/servers:
 *
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of IBM nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL IBM OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (C) Red Hat, Inc., 2009, 2010, 2011
 * Copyright (C) Amit Shah <amit.shah@redhat.com>, 2009, 2010, 2011
 */
/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once

#include <stdint.h>
#include <sddf/serial/queue.h>
#include <libvmm/virtio/mmio.h>

#define RX_QUEUE 0
#define TX_QUEUE 1
#define CTL_RX_QUEUE 2
#define CTL_TX_QUEUE 3

/* For the console device to function, we only need an RX queue and TX queue. */
#define VIRTIO_CONSOLE_NUM_VIRTQ 2

/* Feature bits */
#define VIRTIO_CONSOLE_F_SIZE           0   /* Does host provide console size? */
#define VIRTIO_CONSOLE_F_MULTIPORT      1   /* Does host provide multiple ports? */
#define VIRTIO_CONSOLE_F_EMERG_WRITE    2   /* Does host support emergency write? */

#define VIRTIO_CONSOLE_BAD_ID (~(uint32_t)0)

#define VIRTIO_CONSOLE_MAX_PORTS 4 /* Support max 4 ports right now... */

#define VIRTIO_CONSOLE_CFG_MAX_PORTS (VIRTIO_PCI_CONFIG_OFF(false) + offsetof(struct virtio_con_cfg, max_nr_ports))

struct virtio_console_config {
    /* colums of the screens */
    uint16_t cols;
    /* rows of the screens */
    uint16_t rows;
    /* max. number of ports this device can hold */
    uint32_t max_nr_ports;
    /* emergency write register */
    uint32_t emerg_wr;
} __attribute__((packed));

/*
 * A message that's passed between the Host and the Guest for a
 * particular port.
 */
struct virtio_console_control {
    uint32_t id;    /* Port number */
    uint16_t event; /* The kind of control event (see below) */
    uint16_t value; /* Extra information for the key */
} __attribute__((packed));

/* Some events for control messages */
#define VIRTIO_CONSOLE_DEVICE_READY 0
#define VIRTIO_CONSOLE_PORT_ADD     1
#define VIRTIO_CONSOLE_PORT_REMOVE  2
#define VIRTIO_CONSOLE_PORT_READY   3
#define VIRTIO_CONSOLE_CON_PORT     4
#define VIRTIO_CONSOLE_RESIZE       5
#define VIRTIO_CONSOLE_PORT_OPEN    6
#define VIRTIO_CONSOLE_PORT_NAME    7

struct virtio_console_device {
    struct virtio_device virtio_device;
    struct virtio_queue_handler vqs[VIRTIO_CONSOLE_NUM_VIRTQ];
    serial_queue_handle_t *rxq;
    serial_queue_handle_t *txq;
    int tx_ch;
};

bool virtio_mmio_console_init(struct virtio_console_device *console,
                              uintptr_t region_base,
                              uintptr_t region_size,
                              size_t virq,
                              serial_queue_handle_t *rxq,
                              serial_queue_handle_t *txq,
                              int tx_ch);

bool virtio_console_handle_rx(struct virtio_console_device *console);
