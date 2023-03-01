/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/**
 * @brief virtio vswitch backend layer
 *
 * The virtio vswitch backend has two layers, the emul layer and the backend layer.
 * This file is part of the backend layer, it's responsible for:
 *
 * - TX: receiving data from the emul layer and forwarding them to the destination
 *   VM using sharedringbuffer
 * - RX: receiving data from the source VM via sharedringbuffer and invoking the
 *   emul layer to handle the data
 * - virtio vswitch backend initialization
 *
 * Data flow:
 *
 *          vq                      shringbuf                      vq
 *  src VM <--> (emul <-> backend) <---------> (backend <-> emul) <--> dest VM
 *                  src VMM                          dest VMM
 *
 * @todo This backend relies on a connection topology provided by maybe a build system,
 * currently we haven't figured out what to do.
 */

// #include <string.h>
#include "include/config/virtio_net.h"
#include "virtio_net_vswitch.h"
#include "../util/util.h"

#include "../libsharedringbuffer/include/shared_ringbuffer.h"

#define SHMEM_NUM_BUFFERS 256
#define SHMEM_BUF_SIZE 0x1000

typedef struct node_handler {
    ring_handle_t rx_ring;
    ring_handle_t tx_ring;
    uint8_t macaddr[6];
    sel4cp_channel channel;
} node_handler_t;

// @jade: Need to introduce or paramterise this variable into the build system
#define NUM_NODE 1

// handler of the connections
node_handler_t map[NUM_NODE];

// @jade: should be able to get this from a build system in the future
uint8_t vswitch_mac_address[6];

// rx callback provided by the client
callback_fn_t client_cb;

#define PR_MAC802_ADDR  "%x:%x:%x:%x:%x:%x"

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
} __attribute__ ((__packed__));

// static uint8_t null_macaddr[6] = {0, 0, 0, 0, 0, 0};
static uint8_t bcast_macaddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t ipv6_multicast_macaddr[6] = {0x33, 0x33, 0x0, 0x0, 0x0, 0x0};

static inline bool mac802_addr_eq_num(uint8_t *addr0, uint8_t *addr1, unsigned int num)
{
    assert(num <= 6);
    for (int i = 0; i < num; i++) {
        if (addr0[i] != addr1[i]) {
            return false;
        }
    }
    return true;
}

static inline bool mac802_addr_eq(uint8_t *addr0, uint8_t *addr1)
{
    return mac802_addr_eq_num(addr0, addr1, 6);
}

static inline bool mac802_addr_eq_bcast(uint8_t *addr)
{
    return mac802_addr_eq(addr, bcast_macaddr);
}

static inline bool mac802_addr_eq_ipv6_mcast(uint8_t *addr)
{
    return mac802_addr_eq_num(addr, ipv6_multicast_macaddr, 2);
}

void vswitch_get_mac(uint8_t *retval)
{
    for (int i = 0; i < 6; i++) {
        retval[i] = vswitch_mac_address[i];
    }
}

node_handler_t *get_node_by_mac(uint8_t *addr)
{
    for (int i = 0; i < NUM_NODE; i++) {
        if (mac802_addr_eq(addr, map[i].macaddr)) {
            return &map[i];
        }
    }
    return NULL;
}

node_handler_t *get_node_by_channel(sel4cp_channel ch)
{
    for (int i = 0; i < NUM_NODE; i++) {
        if (map[i].channel == ch) {
            return &map[i];
        }
    }
    return NULL;
}

static int send_packet_to_node(void *buf, uint32_t size, node_handler_t *node)
{
    assert(node);

    uintptr_t addr;
    unsigned int len;
    void *cookie;

    // get a buffer from the avail ring
    int error = dequeue_avail(&node->tx_ring, &addr, &len, &cookie);
    if (error) {
        printf("\"%s\"|VSWITCH|INFO: avail ring is empty\n", sel4cp_name);
        return 1;
    }
    assert(size <= len);

    // copy data to the buffer
    memcpy((void *)addr, buf, size);

    /* insert into the used ring */
    error = enqueue_used(&node->tx_ring, addr, size, NULL);
    if (error) {
        printf("\"%s\"|VSWITCH|INFO: TX used ring full\n", sel4cp_name);
        enqueue_avail(&node->tx_ring, addr, len, NULL);
        return 1;
    }

    /* notify the other end of the node */
    sel4cp_notify(node->channel);

    return 0;
}

// sent packet from this vmm to another
int vswitch_tx(void *buf, uint32_t size)
{
    /* Initialize a convenience pointer to the dest macaddr.
     * The dest MAC addr is the first member of an ethernet frame. */
    struct ether_addr *macaddr = (struct ether_addr *)buf;

    // san check
    if (!mac802_addr_eq(macaddr->ether_src_addr_octet, vswitch_mac_address)) {
        printf("\"%s\"|VSWITCH|INFO: incorrect src MAC: "PR_MAC802_ADDR"\n", sel4cp_name, PR_MAC802_SRC_ADDR_ARGS(macaddr));
        return 1;
    }

    int error = 0;

    // printf("\"%s\"|VSWITCH|INFO: transmitting, dest MAC: "PR_MAC802_ADDR", src MAC: "PR_MAC802_ADDR"\n",
    //         sel4cp_name, PR_MAC802_DEST_ADDR_ARGS(macaddr), PR_MAC802_SRC_ADDR_ARGS(macaddr));

    // if the mac address is broadcast of ipv6 milticast, sent the packet to everyone
    if (mac802_addr_eq_bcast(macaddr->ether_dest_addr_octet) || mac802_addr_eq_ipv6_mcast(macaddr->ether_dest_addr_octet)) {
        for (int i = 0; i < NUM_NODE; i++) {
            error += send_packet_to_node(buf, size, &map[i]);
        }

    // otherwise, find the receiver
    } else {
        node_handler_t *node = get_node_by_mac(macaddr->ether_dest_addr_octet);
        if (node) {
            error = send_packet_to_node(buf, size, node);
        }
    }
    // we drop the packet if we can't find a destination for it

    return error;
}

int vswitch_rx(sel4cp_channel channel)
{
    uintptr_t addr;
    unsigned int len;
    void *cookie;

    // struct ether_addr *macaddr = (struct ether_addr *)addr;
    // printf("\"%s\"|VIRTIO MMIO|INFO: incoming, dest MAC: "PR_MAC802_ADDR", src MAC: "PR_MAC802_ADDR"\n",
    //         sel4cp_name, PR_MAC802_DEST_ADDR_ARGS(macaddr), PR_MAC802_SRC_ADDR_ARGS(macaddr));

    node_handler_t *node = get_node_by_channel(channel);
    assert(node);

    while(!ring_empty(node->rx_ring.used_ring)) {

        int error = dequeue_used(&node->rx_ring, &addr, &len, &cookie);
        // RX used ring is empty, this is not suppose to happend!
        assert(!error);

        client_cb((void *)addr, len);

        enqueue_avail(&node->rx_ring, addr, SHMEM_BUF_SIZE, NULL);
    }

    return 0;
}

eth_driver_handler_t vswitch_handler = {
    .get_mac = vswitch_get_mac,
    .tx = vswitch_tx,
    .rx = vswitch_rx,
};

/* Memory regions. These all have to be here to keep compiler happy */
uintptr_t rx_avail;
uintptr_t rx_used;
uintptr_t tx_avail;
uintptr_t tx_used;
uintptr_t rx_shared_dma_vaddr;
uintptr_t tx_shared_dma_vaddr;

eth_driver_handler_t *vswitch_init(callback_fn_t cb)
{
    // @jade: we don't have a good way to config connection layouts for the vswitch right now,
    // so I'm just going to hard code a node to get the vswitch works.

    ring_init(&map[0].rx_ring, (ring_buffer_t *)rx_avail, (ring_buffer_t *)rx_used, NULL, 1);
    ring_init(&map[0].tx_ring, (ring_buffer_t *)tx_avail, (ring_buffer_t *)tx_used, NULL, 0);

    map[0].macaddr[0] = 0x02;
    map[0].macaddr[1] = 0x00;
    map[0].macaddr[2] = 0x00;
    map[0].macaddr[3] = 0x00;
    map[0].macaddr[4] = 0xaa;

    vswitch_mac_address[0] = 0x02;
    vswitch_mac_address[1] = 0x00;
    vswitch_mac_address[2] = 0x00;
    vswitch_mac_address[3] = 0x00;
    vswitch_mac_address[4] = 0xaa;

    if (get_vmm_id(sel4cp_name) == 1) {
        map[0].macaddr[5] = 0x02;
        vswitch_mac_address[5] = 0x01;

    } else {
        map[0].macaddr[5] = 0x01;
        vswitch_mac_address[5] = 0x02;
    }

    map[0].channel = VSWITCH_CONN_1_CH;

    // fill avail queue with empty buffers
    for (int i = 0; i < SHMEM_NUM_BUFFERS - 1; i++) {
        uintptr_t addr = rx_shared_dma_vaddr + (SHMEM_BUF_SIZE * i);
        int ret = enqueue_avail(&map[0].rx_ring, addr, SHMEM_BUF_SIZE, NULL);
        assert(ret == 0);
    }

    client_cb = cb;

    return &vswitch_handler;
}