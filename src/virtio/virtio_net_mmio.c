/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*
* This file is meant to convert virtio virtqueue structures into sDDF shared ringbuffer structures.
* However, it currently implements a vswitch that connects two vmm instances together, and ideally should talk
* to a net multiplexer to determine who to send the packet to. A major refactor is needed to move
* multiplexer logic out of this file.
*/

/**
 * @brief virtio net vswitch implementation
 *
 * - TX: transfers virtio net virtqueue data to sDDF transport layer, and is
 *       delivered to the destination VM using sharedringbuffer
 * - RX: receives data from the source VM via sharedringbuffer and invokes handler
 *       to convert the data from sDDF transport layer to virtio net virtqueue data
 * - Initialisation of virtio_net and vswitch implementation
 *
 * Data flow:
 *
 *          vq                      shringbuf                      vq
 *  src VM <--> (virtio net mmio) <-----------> (virtio net mmio) <--> dest VM
 *                  src VMM                          dest VMM
 *
 * @todo This implementation relies on a connection topology provided by maybe a build system,
 * currently we haven't figured out what to do.
 */

#include <stddef.h>
#include "virtio_mmio.h"
#include "virtio_net_mmio.h"
#include "virtio/virtio_irq.h"
#include "include/config/virtio_net.h"
#include "include/config/virtio_config.h"
#include "../virq.h"
#include "../util/util.h"
#include "../libsharedringbuffer/include/shared_ringbuffer.h"

// @jade: add some giant comments about this file

#define BIT_LOW(n) (1ul<<(n))
#define BIT_HIGH(n) (1ul<<(n - 32 ))

#define REG_RANGE(r0, r1)   r0 ... (r1 - 1)

#define RX_QUEUE 0
#define TX_QUEUE 1

// @jade, @ericc: These are sDDF specific, belong in a configuration file elsewhere ideally
#define SHMEM_NUM_BUFFERS 256
#define SHMEM_BUF_SIZE 0x1000

// @jade, @ivanv: need to be able to get it from vgic
#define VCPU_ID 0

// @jade: random number that I picked, maybe a smaller buffer size would be better?
#define TMP_BUF_SIZE 0x1000

// @jade: Need to introduce or parameterise this variable into the build system
#define NUM_NODE 1

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

// mmio handler of this instance of virtio net layer
static virtio_mmio_handler_t net_mmio_handler;

// the list of virtqueue handlers for this instance of virtio net layer
static virtqueue_t vqs[VIRTIO_NET_MMIO_NUM_VIRTQUEUE];

// temporary buffer to transmit buffer from this layer to the backend layer
static char temp_buf[TMP_BUF_SIZE];

// sDDF memory regions, initialised by the virtio net mmio init function
static uintptr_t rx_avail;
static uintptr_t rx_used;
static uintptr_t tx_avail;
static uintptr_t tx_used;
static uintptr_t rx_shared_dma_vaddr;
static uintptr_t tx_shared_dma_vaddr;

typedef struct node_handler {
    ring_handle_t rx_ring;
    ring_handle_t tx_ring;
    uint8_t macaddr[6];
    sel4cp_channel channel;
} node_handler_t;

// handler of the connections
static node_handler_t map[NUM_NODE];

// @jade: should be able to get this from a build system in the future
static uint8_t vswitch_mac_address[6];

/* This is a name for the 96 bit ethernet addresses available on many
   systems.  */
struct ether_addr {
    uint8_t ether_dest_addr_octet[6];
    uint8_t ether_src_addr_octet[6];
} __attribute__ ((__packed__));

// static uint8_t null_macaddr[6] = {0, 0, 0, 0, 0, 0};
static uint8_t bcast_macaddr[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t ipv6_multicast_macaddr[6] = {0x33, 0x33, 0x0, 0x0, 0x0, 0x0};

static void vswitch_get_mac(uint8_t *retval);

static int vswitch_tx(void *buf, uint32_t size);

static void vswitch_init(void);

void virtio_net_mmio_ack(uint64_t vcpu_id, int irq, void *cookie) {
    // printf("\"%s\"|VIRTIO NET|INFO: virtio_net_ack %d\n", sel4cp_name, irq);
    // nothing needs to be done
}

virtio_mmio_handler_t *get_virtio_net_mmio_handler(void)
{
    // san check in case somebody wants to get the handler of an uninitialised backend
    if (net_mmio_handler.data.VendorID != VIRTIO_MMIO_DEV_VENDOR_ID) {
        return NULL;
    }
    return &net_mmio_handler;
}

void virtio_net_mmio_reset(void)
{
    vqs[RX_QUEUE].ready = 0;
    vqs[RX_QUEUE].last_idx = 1;

    vqs[TX_QUEUE].ready = 0;
    vqs[TX_QUEUE].last_idx = 0;
}

int virtio_net_mmio_get_device_features(uint32_t *features)
{
    if (net_mmio_handler.data.Status & VIRTIO_CONFIG_S_FEATURES_OK) {
        printf("VIRTIO NET|WARNING: driver somehow wants to read device features after FEATURES_OK\n");
    }

    switch (net_mmio_handler.data.DeviceFeaturesSel) {
        // feature bits 0 to 31
        case 0:
            *features = BIT_LOW(VIRTIO_NET_F_MAC);
            break;
        // features bits 32 to 63
        case 1:
            *features = BIT_HIGH(VIRTIO_F_VERSION_1);
            break;
        default:
            printf("VIRTIO NET|INFO: driver sets DeviceFeaturesSel to 0x%x, which doesn't make sense\n", net_mmio_handler.data.DeviceFeaturesSel);
            return 0;
    }
    return 1;
}

int virtio_net_mmio_set_driver_features(uint32_t features)
{
    int success = 1;

    switch (net_mmio_handler.data.DriverFeaturesSel) {
        // feature bits 0 to 31
        case 0:
            // The device initialisation protocol says the driver should read device feature bits,
            // and write the subset of feature bits understood by the OS and driver to the device.
            // Currently we only have one feature to check.
            success = (features == BIT_LOW(VIRTIO_NET_F_MAC));
            break;

        // features bits 32 to 63
        case 1:
            success = (features == BIT_HIGH(VIRTIO_F_VERSION_1));
            break;

        default:
            printf("VIRTIO NET|INFO: driver sets DriverFeaturesSel to 0x%x, which doesn't make sense\n", net_mmio_handler.data.DriverFeaturesSel);
            success = 0;
    }
    if (success) {
        net_mmio_handler.data.features_happy = 1;
    }
    return success;
}

int virtio_net_mmio_get_device_config(uint32_t offset, uint32_t *ret_val)
{
    // @jade: this function might need a refactor when the virtio net backend starts to
    // support more features
    switch (offset) {
        // get mac low
        case REG_RANGE(0x100, 0x104):
        {
            uint8_t mac[6];
            vswitch_get_mac(mac);
            *ret_val = mac[0];
            *ret_val |= mac[1] << 8;
            *ret_val |= mac[2] << 16;
            *ret_val |= mac[3] << 24;
            break;
        }
        // get mac high
        case REG_RANGE(0x104, 0x108):
        {
            uint8_t mac[6];
            vswitch_get_mac(mac);
            *ret_val = mac[4];
            *ret_val |= mac[5] << 8;
            break;
        }
        default:
            printf("VIRTIO NET|WARNING: unknown device config register: 0x%x\n", offset);
            return 0;
    }
    return 1;
}

int virtio_net_mmio_set_device_config(uint32_t offset, uint32_t val)
{
    printf("VIRTIO NET|WARNING: driver attempts to set device config but virtio net only has driver-read-only configuration fields\n");
    return 0;
}

// notify the guest VM that we successfully delivered their packet
static void virtio_net_mmio_tx_complete(uint16_t desc_head)
{
        // set the reason of the irq
        net_mmio_handler.data.InterruptStatus = BIT_LOW(0);

        bool success = virq_inject(VCPU_ID, VIRTIO_NET_IRQ);
        // we can't inject irqs?? panic.
        assert(success);

        //add to useds
        struct vring *vring = &vqs[TX_QUEUE].vring;

        struct vring_used_elem used_elem = {desc_head, 0};
        uint16_t guest_idx = vring->used->idx;

        vring->used->ring[guest_idx % vring->num] = used_elem;
        vring->used->idx++;
}

// handle queue notify from the guest VM
static int virtio_net_mmio_handle_queue_notify_tx()
{
    struct vring *vring = &vqs[TX_QUEUE].vring;

    /* read the current guest index */
    uint16_t guest_idx = vring->avail->idx;
    uint16_t idx = vqs[TX_QUEUE].last_idx;

    for (; idx != guest_idx; idx++) {
        /* read the head of the descriptor chain */
        uint16_t desc_head = vring->avail->ring[idx % vring->num];

        /* byte written */
        uint32_t written = 0;

        /* we want to skip the initial virtio header, as this should
         * not be sent to the actual ethernet driver. This records
         * how much we have skipped so far. */
        uint32_t skipped = 0;

        uint16_t curr_desc_head = desc_head;

        do {
            uint32_t skipping = 0;
            /* if we haven't yet skipped the full virtio net header, work
             * out how much of this descriptor should be skipped */
            if (skipped < sizeof(struct virtio_net_hdr_mrg_rxbuf)) {
                skipping = MIN(sizeof(struct virtio_net_hdr_mrg_rxbuf) - skipped, vring->desc[curr_desc_head].len);
                skipped += skipping;
            }

            /* truncate packets that are large than BUF_SIZE */
            uint32_t writing = MIN(TMP_BUF_SIZE - written, vring->desc[curr_desc_head].len - skipping);

            // @jade: we want to eliminate this copy
            memcpy(temp_buf + written, (void *)vring->desc[curr_desc_head].addr + skipping, writing);
            written += writing;
            curr_desc_head = vring->desc[curr_desc_head].next;
        } while (vring->desc[curr_desc_head].flags & VRING_DESC_F_NEXT);

        /* ship the buffer to the next layer */
        int err = vswitch_tx(temp_buf, written);
        if (err) {
            printf("VIRTIO NET|WARNING: VirtIO Net failed to deliver packet for the guest\n.");
        }

        virtio_net_mmio_tx_complete(desc_head);
    }

    vqs[TX_QUEUE].last_idx = idx;

    return 1;
}

// handle rx from vswitch
static int virtio_net_mmio_handle_vswitch_rx(void *buf, uint32_t size)
{
    if (!vqs[RX_QUEUE].ready) {
        // vq is not initialised, drop the packet
        return 1;
    }
    struct vring *vring = &vqs[RX_QUEUE].vring;

    /* grab the next receive chain */
    uint16_t guest_idx = vring->avail->idx;
    uint16_t idx = vqs[RX_QUEUE].last_idx;

    if (idx == guest_idx) {
        printf("\"%s\"|VIRTIO NET|WARNING: queue is full, drop the packet\n", sel4cp_name);
        return 1;
    }

    struct virtio_net_hdr_mrg_rxbuf virtio_hdr;
    memzero(&virtio_hdr, sizeof(virtio_hdr));

    /* total length of the copied packet */
    size_t copied = 0;
    /* amount of the current descriptor copied */
    size_t desc_copied = 0;

    /* read the head of the descriptor chain */
    uint16_t desc_head = vring->avail->ring[idx % vring->num];
    uint16_t curr_desc_head = desc_head;

    // have we finished copying the net header?
    bool net_header_processed = false;

    do {
        /* determine how much we can copy */
        uint32_t copying;
        /* what are we copying? */
        void *buf_base;

        // process the net header
        if (!net_header_processed) {
            copying = sizeof(struct virtio_net_hdr_mrg_rxbuf) - copied;
            buf_base = &virtio_hdr;

        // otherwise, process the actual packet
        } else {
            copying = size - copied;
            buf_base = buf;
        }

        copying = MIN(copying, vring->desc[curr_desc_head].len - desc_copied);

        memcpy((void *)vring->desc[curr_desc_head].addr + desc_copied, buf_base + copied, copying);

        /* update amounts */
        copied += copying;
        desc_copied += copying;

        // do we need another buffer from the virtqueue?
        if (desc_copied == vring->desc[curr_desc_head].len) {
            if (!vring->desc[curr_desc_head].flags & VRING_DESC_F_NEXT) {
                /* descriptor chain is too short to hold the whole packet.
                * just truncate */
                break;
            }
            curr_desc_head = vring->desc[curr_desc_head].next;
            desc_copied = 0;
        }

        // have we finished copying the net header?
        if (copied == sizeof(struct virtio_net_hdr_mrg_rxbuf)) {
            copied = 0;
            net_header_processed = true;
        }

    } while (!net_header_processed || copied < size);

    // record the real amount we have copied
    if (net_header_processed) {
        copied += sizeof(struct virtio_net_hdr_mrg_rxbuf);
    }
    /* now put it in the used ring */
    struct vring_used_elem used_elem = {desc_head, copied};
    uint16_t used_idx = vring->used->idx;

    vring->used->ring[used_idx % vring->num] = used_elem;
    vring->used->idx++;

    /* record that we've used this descriptor chain now */
    vqs[RX_QUEUE].last_idx++;

    // set the reason of the irq
    virtio_mmio_handler_t *handler = get_virtio_net_mmio_handler();
    assert(handler);
    handler->data.InterruptStatus = BIT_LOW(0);

    // notify the guest
    bool success = virq_inject(VCPU_ID, VIRTIO_NET_IRQ);
    // we can't inject irqs?? panic.
    assert(success);

    return 0;
}

static virtio_mmio_funs_t mmio_funs = {
    .device_reset = virtio_net_mmio_reset,
    .get_device_features = virtio_net_mmio_get_device_features,
    .set_driver_features = virtio_net_mmio_set_driver_features,
    .get_device_config = virtio_net_mmio_get_device_config,
    .set_device_config = virtio_net_mmio_set_device_config,
    .queue_notify = virtio_net_mmio_handle_queue_notify_tx,
};

void virtio_net_mmio_init(uintptr_t net_tx_avail, uintptr_t net_tx_used, uintptr_t net_tx_shared_dma_vaddr,
                          uintptr_t net_rx_avail, uintptr_t net_rx_used, uintptr_t net_rx_shared_dma_vaddr)
{    
    tx_avail = net_tx_avail;
    tx_used = net_tx_used;
    tx_shared_dma_vaddr = net_tx_shared_dma_vaddr;
    rx_avail = net_rx_avail;
    rx_used = net_rx_used;
    rx_shared_dma_vaddr = net_rx_shared_dma_vaddr;

    vswitch_init();

    net_mmio_handler.data.DeviceID = DEVICE_ID_VIRTIO_NET;
    net_mmio_handler.data.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    net_mmio_handler.funs = &mmio_funs;

    /* must keep this or the driver complains */
    vqs[RX_QUEUE].last_idx = 1;

    net_mmio_handler.vqs = vqs;
}

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

static void vswitch_get_mac(uint8_t *retval)
{
    for (int i = 0; i < 6; i++) {
        retval[i] = vswitch_mac_address[i];
    }
}

static node_handler_t *get_node_by_mac(uint8_t *addr)
{
    for (int i = 0; i < NUM_NODE; i++) {
        if (mac802_addr_eq(addr, map[i].macaddr)) {
            return &map[i];
        }
    }
    return NULL;
}

static node_handler_t *get_node_by_channel(sel4cp_channel ch)
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
static int vswitch_tx(void *buf, uint32_t size)
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

        virtio_net_mmio_handle_vswitch_rx((void *)addr, len);

        enqueue_avail(&node->rx_ring, addr, SHMEM_BUF_SIZE, NULL);
    }

    return 0;
}

// @ericc: Leaving this hack here for now, refactor in the future
// This is specfic to the vswitch, we need to know which vmm we are
static uint64_t get_vmm_id(char *sel4cp_name)
{
    // @ivanv: Absolute hack
    return sel4cp_name[4] - '0';
}

static void vswitch_init()
{
    // @jade: we don't have a good way to config connection layouts for the vswitch right now,
    // so I'm just going to hard-code a node to get the vswitch working.
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

    map[0].channel = VSWITCH_CONN_CH_1;

    // fill avail queue with empty buffers
    for (int i = 0; i < SHMEM_NUM_BUFFERS - 1; i++) {
        uintptr_t addr = rx_shared_dma_vaddr + (SHMEM_BUF_SIZE * i);
        int ret = enqueue_avail(&map[0].rx_ring, addr, SHMEM_BUF_SIZE, NULL);
        assert(ret == 0);
    }
}