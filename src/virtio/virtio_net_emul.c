/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stddef.h>
#include "virtio_mmio.h"
// @jade: find a way to config which backend (vswitch/ethernet/guest driver) to include
#include "virtio_net_vswitch.h"
#include "virtio_net_emul.h"
#include "include/config/virtio_net.h"
#include "include/config/virtio_config.h"
#include "../arch/aarch64/vgic/vgic.h"

// @jade: add some giant comments about this file
// virtio net mmio emul interface

// @jade, @ivanv: need to be able to get it from vgic
#define VCPU_ID 0

// @jade: ideally should not be here
#define VIRTIO_NET_IRQ 74

#define BIT_LOW(n) (1ul<<(n))
#define BIT_HIGH(n) (1ul<<(n - 32 ))

#define RX_QUEUE 0
#define TX_QUEUE 1

// @jade: random number that I picked, maybe a smaller buffer size would be better?
#define BUF_SIZE 0x1000

#define REG_RANGE(r0, r1)   r0 ... (r1 - 1)

// emul handler of this instant of virtio net emul layer
virtio_emul_handler_t net_emul_handler;

// the handler of the backend that this instant of virtio net emul layer talks to
eth_driver_handler_t *backend_handler;

// the list of virtqueue handlers for this instant of virtio net emul layer
virtqueue_t vqs[VIRTIO_MMIO_NET_NUM_VIRTQUEUE];

// temporary buffer to transmit buffer from this layer to the backend layer
char temp_buf[BUF_SIZE];

void virtio_net_ack(uint64_t vcpu_id, int irq, void *cookie) {
    // printf("\"%s\"|VIRTIO NET|INFO: virtio_net_ack %d\n", sel4cp_name, irq);
    // nothing needs to be done
}

virtio_emul_handler_t *get_virtio_net_emul_handler(void)
{
    // san check in case somebody wants to get the handler of an uninitialised backend
    if (net_emul_handler.data.VendorID != VIRTIO_MMIO_DEV_VENDOR_ID) {
        return NULL;
    }
    return &net_emul_handler;
}

static void virtio_net_emul_reset(virtio_emul_handler_t *self)
{
    vqs[RX_QUEUE].ready = 0;
    vqs[RX_QUEUE].last_idx = 1;

    vqs[TX_QUEUE].ready = 0;
    vqs[TX_QUEUE].last_idx = 0;
}

static int virtio_net_emul_get_device_features(virtio_emul_handler_t *self, uint32_t *features)
{
    if (self->data.Status & VIRTIO_CONFIG_S_FEATURES_OK) {
        printf("VIRTIO NET|WARNING: driver somehow wants to read device features after FEATURES_OK\n");
    }

    switch (self->data.DeviceFeaturesSel) {
        // feature bits 0 to 31
        case 0:
            *features = BIT_LOW(VIRTIO_NET_F_MAC);
            break;
        // features bits 32 to 63
        case 1:
            *features = BIT_HIGH(VIRTIO_F_VERSION_1);
            break;
        default:
            printf("VIRTIO NET|INFO: driver sets DeviceFeaturesSel to 0x%x, which doesn't make sense\n", self->data.DeviceFeaturesSel);
            return 0;
    }
    return 1;
}

static int virtio_net_emul_set_driver_features(virtio_emul_handler_t *self, uint32_t features)
{
    int success = 1;

    switch (self->data.DriverFeaturesSel) {
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
            printf("VIRTIO NET|INFO: driver sets DriverFeaturesSel to 0x%x, which doesn't make sense\n", self->data.DriverFeaturesSel);
            success = 0;
    }
    if (success) {
        self->data.features_happy = 1;
    }
    return success;
}

static int virtio_net_emul_get_device_config(struct virtio_emul_handler *self, uint32_t offset, uint32_t *ret_val)
{
    // @jade: this function might need a refactor when the virtio net backend starts to
    // support more features
    switch (offset) {
        // get mac low
        case REG_RANGE(0x100, 0x104):
        {
            uint8_t mac[6];
            backend_handler->get_mac(mac);
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
            backend_handler->get_mac(mac);
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

static int virtio_net_emul_set_device_config(struct virtio_emul_handler *self, uint32_t offset, uint32_t val)
{
    printf("VIRTIO NET|WARNING: driver attempts to set device config but virtio net only has driver-read-only configuration fields\n");
    return 0;
}

// notify the guest that we successfully deliver their packet
static void virtio_net_emul_tx_complete(struct virtio_emul_handler *self, uint16_t desc_head)
{
        // set the reason of the irq
        self->data.InterruptStatus = BIT_LOW(0);

        bool success = vgic_inject_irq(VCPU_ID, VIRTIO_NET_IRQ);
        // we can't inject irqs?? panic.
        assert(success);

        //add to useds
        struct vring *vring = &vqs[TX_QUEUE].vring;

        struct vring_used_elem used_elem = {desc_head, 0};
        uint16_t guest_idx = vring->used->idx;

        vring->used->ring[guest_idx % vring->num] = used_elem;
        vring->used->idx++;
}

// handle queue notify from the guest
static int virtio_net_emul_handle_queue_notify_tx(struct virtio_emul_handler *self)
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
            uint32_t writing = MIN(BUF_SIZE - written, vring->desc[curr_desc_head].len - skipping);

            // @jade: we want to eliminate this copy
            memcpy(temp_buf + written, (void *)vring->desc[curr_desc_head].addr + skipping, writing);
            written += writing;
            curr_desc_head = vring->desc[curr_desc_head].next;
        } while (vring->desc[curr_desc_head].flags & VRING_DESC_F_NEXT);

        /* ship the buffer to the next layer */
        int err = backend_handler->tx(temp_buf, written);
        if (err) {
            printf("VIRTIO NET|WARNING: VirtIO Net failed to deliver packet for the guest\n.");
        }

        virtio_net_emul_tx_complete(self, desc_head);
    }

    vqs[TX_QUEUE].last_idx = idx;

    return 1;
}

// handle rx for the backend
static int virtio_net_emul_handle_backend_rx(void *buf, uint32_t size)
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
        void *buf_base = NULL;

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
    struct virtio_emul_handler *handler = get_virtio_net_emul_handler();
    assert(handler);
    handler->data.InterruptStatus = BIT_LOW(0);

    // notify the guest
    bool success = vgic_inject_irq(VCPU_ID, VIRTIO_NET_IRQ);
    // we can't inject irqs?? panic.
    assert(success);

    return 0;
}

virtio_emul_funs_t emul_funs = {
    .device_reset = virtio_net_emul_reset,
    .get_device_features = virtio_net_emul_get_device_features,
    .set_driver_features = virtio_net_emul_set_driver_features,
    .get_device_config = virtio_net_emul_get_device_config,
    .set_device_config = virtio_net_emul_set_device_config,
    .queue_notify = virtio_net_emul_handle_queue_notify_tx,
};

void virtio_net_emul_init(void)
{
    backend_handler = vswitch_init(virtio_net_emul_handle_backend_rx);
    if (backend_handler == NULL) {
        printf("VIRTIO NET|WARNING: VirtIO Net is initialised unsuccessfully\n.");
        return;
    }

    net_emul_handler.data.DeviceID = DEVICE_ID_VIRTIO_NET;
    net_emul_handler.data.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    net_emul_handler.funs = &emul_funs;

    /* must keep this or the driver complains */
    vqs[RX_QUEUE].last_idx = 1;

    net_emul_handler.vqs = vqs;
}