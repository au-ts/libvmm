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
#include "virtio/mmio.h"

#define RX_QUEUE 0
#define TX_QUEUE 1

// it is set to 2 because the backend currently don't support VIRTIO_NET_F_MQ and VIRTIO_NET_F_CTRL_VQ,
// we (@jade) might work on supporting them in the future
#define VIRTIO_NET_NUM_VIRTQ 2

#pragma once

#define PR_MAC802_ADDR  "%x:%x:%x:%x:%x:%x"

// @jade: all these helper code should not be part of the virtio lib. Need to figure
// out a nice place to put them into

/* Expects a *pointer* to a struct ether_addr */
#define PR_MAC802_ADDR_ARGS(a, dir)     (a)->ether_##dir##_addr_octet[0], \
                                        (a)->ether_##dir##_addr_octet[1], \
                                        (a)->ether_##dir##_addr_octet[2], \
                                        (a)->ether_##dir##_addr_octet[3], \
                                        (a)->ether_##dir##_addr_octet[4], \
                                        (a)->ether_##dir##_addr_octet[5]

#define PR_MAC802_DEST_ADDR_ARGS(a) PR_MAC802_ADDR_ARGS(a, dest)
#define PR_MAC802_SRC_ADDR_ARGS(a) PR_MAC802_ADDR_ARGS(a, src)

/* This is a name for the 96 bit ethernet addresses available on many
   systems.  */
struct ether_addr {
    uint8_t ether_dest_addr_octet[6];
    uint8_t ether_src_addr_octet[6];
    uint8_t etype[2]; // Ethertype
    uint8_t payload[46];
    uint8_t crc[4];
} __attribute__ ((__packed__));

uint8_t null_macaddr[6] = {0, 0, 0, 0, 0, 0};
uint8_t bcast_macaddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t ipv6_multicast_macaddr[6] = {0x33, 0x33, 0x0, 0x0, 0x0, 0x0};

inline bool mac802_addr_eq_num(uint8_t *addr0, uint8_t *addr1, unsigned int num)
{
    for (int i = 0; i < num; i++) {
        if (addr0[i] != addr1[i]) {
            return false;
        }
    }
    return true;
}

inline bool mac802_addr_eq(uint8_t *addr0, uint8_t *addr1)
{
    return mac802_addr_eq_num(addr0, addr1, 6);
}

inline bool mac802_addr_eq_bcast(uint8_t *addr)
{
    return mac802_addr_eq(addr, bcast_macaddr);
}

inline bool mac802_addr_eq_ipv6_mcast(uint8_t *addr)
{
    return mac802_addr_eq_num(addr, ipv6_multicast_macaddr, 2);
}

void virtio_net_init(struct virtio_device *dev,
                     struct virtio_queue_handler *vqs, size_t num_vqs,
                     size_t virq,
                     ring_handle_t *sddf_rx_ring, ring_handle_t *sddf_tx_ring,
                     size_t sddf_mux_tx_ch, size_t addf_mux_get_mac_ch);
int net_client_rx(struct virtio_device *dev);
