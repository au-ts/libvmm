/*
 * Copyright 2024, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */


#include "microkit.h"
#include "virtio/config.h"
#include "virtio/virtq.h"
#include "virtio/mmio.h"
#include "virtio/net.h"
#include "virq.h"
#include <util/util.h>
// #include <util/ethernet.h>
#include <sddf/network/queue.h>
#include <sddf/network/mac802.h>

#define REG_RANGE(r0, r1)   r0 ... (r1 - 1)

/* Uncomment this to enable debug logging */
// #define DEBUG_NET

#if defined(DEBUG_NET)
#define LOG_NET(...) do{ printf("VIRTIO(NET): "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_NET(...) do{}while(0)
#endif

#define LOG_NET_ERR(...) do{ printf("VIRTIO(NET)|ERROR: "); printf(__VA_ARGS__); }while(0)
#define LOG_NET_WRN(...) do{ printf("VIRTIO(NET)|WARNING: "); printf(__VA_ARGS__); }while(0)

// @jade: random number that I picked, maybe a smaller buffer size would be better?
#define TMP_BUF_SIZE 2048

static bool virtio_net_handle_rx_buffer(struct virtio_device *dev, void *buf, uint32_t size);

static inline struct virtio_net_device *device_state(struct virtio_device *dev)
{
    return (struct virtio_net_device *)dev->device_data;
}

static void debug_dump_packet(int len, uint8_t *buffer)
{
    struct ether_addr *macaddr = (struct ether_addr *)buffer;
    printf("VIRTIO(NET)|DUMP PACKET:\n");
    printf("dest MAC: "PR_MAC802_ADDR", src MAC: "PR_MAC802_ADDR", type: 0x%02x%02x, size: %d\n",
            PR_MAC802_DEST_ADDR_ARGS(macaddr), PR_MAC802_SRC_ADDR_ARGS(macaddr),
            macaddr->etype[0], macaddr->etype[1], len);
    printf("-------------------- payload begin --------------------\n");
    for (int i = 0; i < len; i++) {
        printf("%02x ", macaddr->payload[i]);
    }
    printf("\n");
    printf("--------------------- payload end ---------------------\n");
    printf("\n");
}

void virtio_net_handle_rx(struct virtio_net_device *state)
{
    struct virtio_device *dev = &state->virtio_device;

    net_buff_desc_t sddf_buffer;
    while(net_dequeue_active(&state->rx, &sddf_buffer) != -1) {
        // @jade: handle errors
        virtio_net_handle_rx_buffer(dev, (void *)sddf_buffer.io_or_offset, sddf_buffer.len);
        net_enqueue_active(&state->rx, sddf_buffer);
    }
}

static void virtio_net_features_print(struct virtio_device *dev, uint32_t features) {
    /* TODO(@jade) */
}

static void virtio_net_reset(struct virtio_device *dev) {
    LOG_NET("operation: reset\n");
    dev->vqs[VIRTIO_NET_RX_VIRTQ].ready = 0;
    dev->vqs[VIRTIO_NET_RX_VIRTQ].last_idx = 0;

    dev->vqs[VIRTIO_NET_TX_VIRTQ].ready = 0;
    dev->vqs[VIRTIO_NET_TX_VIRTQ].last_idx = 0;
}


static int virtio_net_get_device_features(struct virtio_device *dev, uint32_t *features)
{
    LOG_NET("operation: get device features\n");

    if (dev->data.Status & VIRTIO_CONFIG_S_FEATURES_OK) {
        LOG_NET_ERR("driver somehow wants to read device features after FEATURES_OK\n");
    }

    switch (dev->data.DeviceFeaturesSel) {
        // feature bits 0 to 31
        case 0:
            *features = BIT_LOW(VIRTIO_NET_F_MAC);
            break;
        // features bits 32 to 63
        case 1:
            *features = BIT_HIGH(VIRTIO_F_VERSION_1);
            break;
        default:
            LOG_NET_ERR("driver sets DeviceFeaturesSel to 0x%x, which doesn't make sense\n", dev->data.DeviceFeaturesSel);
            return 0;
    }
    return 1;
}


static int virtio_net_set_driver_features(struct virtio_device *dev, uint32_t features)
{
    int success = 1;

    switch (dev->data.DriverFeaturesSel) {
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
            LOG_NET_ERR("driver sets DriverFeaturesSel to 0x%x, which doesn't make sense\n",dev->data.DriverFeaturesSel);
            success = 0;
    }
    if (success) {
        dev->data.features_happy = 1;
    }
    return success;
}

static int virtio_net_get_device_config(struct virtio_device *dev, uint32_t offset, uint32_t *ret_val)
{
    struct virtio_net_device *state = device_state(dev);
    switch (offset) {
        // get mac low
        case REG_RANGE(0x100, 0x104):
        {
            *ret_val = state->config.mac[0];
            *ret_val |= state->config.mac[1] << 8;
            *ret_val |= state->config.mac[2] << 16;
            *ret_val |= state->config.mac[3] << 24;
            break;
        }
        // get mac high
        case REG_RANGE(0x104, 0x108):
        {
            *ret_val = state->config.mac[4];
            *ret_val |= state->config.mac[5] << 8;
            break;
        }
        default:
            LOG_NET_ERR("unknown device config register: 0x%x\n", offset);
            return 0;
    }
    return 1;
}

static int virtio_net_set_device_config(struct virtio_device *dev, uint32_t offset, uint32_t val)
{
    LOG_NET_ERR("driver attempts to set device config but virtio net only has driver-read-only configuration fields\n");
    return 0;
}

static void virtq_enqueue_used(struct virtq *virtq, uint32_t desc_head, uint32_t bytes_written)
{
    struct virtq_used_elem *used_elem = &virtq->used->ring[virtq->used->idx % virtq->num];
    used_elem->id = desc_head;
    used_elem->len = bytes_written;
    virtq->used->idx++;
}

static void virtio_net_respond(struct virtio_device *dev)
{
    dev->data.InterruptStatus = BIT_LOW(0);
    bool success = virq_inject(GUEST_VCPU_ID, dev->virq);
    assert(success);
}

static void handle_tx_msg(struct virtio_device *dev,
                          struct virtq *virtq,
                          uint16_t desc_head,
                          bool *notify_tx_server,
                          bool *respond_to_client)
{
    struct virtio_net_device *state = device_state(dev);

    net_buff_desc_t sddf_buffer;
    int error = net_dequeue_free(&state->tx, &sddf_buffer);
    if (error) {
        LOG_NET_WRN("avail ring is empty\n");
        goto fail;
    }

    void *dest_buf = state->tx_data + sddf_buffer.io_or_offset;

    uint32_t written = 0;
    uint32_t dest_remaining = sddf_buffer.len;

    /* we want to skip the initial virtio header, as this should
     * not be sent to the actual ethernet driver. This records
     * how much we have skipped so far. */
    uint32_t skip_remaining = sizeof(struct virtio_net_hdr_mrg_rxbuf);

    struct virtq_desc *desc = &virtq->desc[desc_head];

    while (dest_remaining > 0) {
        uint32_t skipping = 0;
        /* work out how much of this descriptor must be skipped */
        skipping = MIN(skip_remaining, desc->len);
        /* truncate packets that are large than BUF_SIZE */
        uint32_t writing = MIN(dest_remaining, desc->len - skipping);

        memcpy(dest_buf + written, (void *)desc->addr + skipping, writing);

        skip_remaining -= skipping;
        written += writing;
        dest_remaining -= writing;

        if (desc->flags & VIRTQ_DESC_F_NEXT) {
            desc = &virtq->desc[desc->next];
        } else {
            break;
        }
    }

    error = net_enqueue_active(&state->tx, sddf_buffer);
    if (error) {
        printf("\"%s\"|VMM NET CLIENT|INFO: TX used ring full\n", microkit_name);
        // TODO: This is not safe as you can't have two produces.
        // Use internal queue instead
        // Also need to respond with some sort of error message.
        net_enqueue_free(&state->tx, sddf_buffer);
        goto fail;
    }

    virtq_enqueue_used(virtq, desc_head, written);
    *respond_to_client = true;
    *notify_tx_server = true;
    return;

fail:
    virtq_enqueue_used(virtq, desc_head, 0);
    *respond_to_client = true;
}

/* handle queue notify from the guest VM */
static int virtio_net_queue_notify(struct virtio_device *dev)
{
    struct virtio_net_device *state = device_state(dev);

    if (dev->data.QueueSel != VIRTIO_NET_TX_VIRTQ) {
        LOG_NET_ERR("Invalid queue\n");
        return 0;
    }

    virtio_queue_handler_t *vq = &dev->vqs[VIRTIO_NET_TX_VIRTQ];
    struct virtq *virtq = &vq->virtq;

    /* read the current guest index */
    uint16_t guest_idx = virtq->avail->idx;
    uint16_t idx = vq->last_idx;

    bool notify_tx_server = false;
    bool respond_to_client = false;

    for (; idx != guest_idx; idx++) {
        uint16_t desc_head = virtq->avail->ring[idx % virtq->num];
        handle_tx_msg(dev, virtq, desc_head, &notify_tx_server, &respond_to_client);
    }

    vq->last_idx = idx;

    if (notify_tx_server) {
        microkit_notify(state->tx_ch);
    }
    if (respond_to_client) {
        virtio_net_respond(dev);
    }

    return 1;
}

// handle rx from client
static bool virtio_net_handle_rx_buffer(struct virtio_device *dev, void *buf, uint32_t size)
{
    if (!dev->vqs[VIRTIO_NET_RX_VIRTQ].ready) {
        // vq is not initialised, drop the packet
        return false;
    }

    virtio_queue_handler_t *vq = &dev->vqs[VIRTIO_NET_RX_VIRTQ];
    struct virtq *virtq = &vq->virtq;

    /* grab the next receive chain */
    uint16_t guest_idx = virtq->avail->idx;
    uint16_t idx = vq->last_idx;

    if (idx == guest_idx) {
        // vq is full or not fully initialised (in this case idx and guest_idx are both 0s), drop the packet
        return false;
    }

    struct virtio_net_hdr_mrg_rxbuf virtio_hdr = {0};
    virtio_hdr.num_buffers = 1;

    /* total length of the copied packet */
    size_t copied = 0;
    /* amount of the current descriptor copied */
    size_t desc_copied = 0;

    /* read the head of the descriptor chain */
    uint16_t desc_head = virtq->avail->ring[idx % virtq->num];
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

        copying = MIN(copying, virtq->desc[curr_desc_head].len - desc_copied);

        memcpy((void *)virtq->desc[curr_desc_head].addr + desc_copied, buf_base + copied, copying);

        /* update amounts */
        copied += copying;
        desc_copied += copying;

        // do we need another buffer from the virtqueue?
        if (desc_copied == virtq->desc[curr_desc_head].len) {
            if (!(virtq->desc[curr_desc_head].flags & VIRTQ_DESC_F_NEXT)) {
                /* descriptor chain is too short to hold the whole packet.
                * just truncate */
                break;
            }
            curr_desc_head = virtq->desc[curr_desc_head].next;
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
    virtq_enqueue_used(virtq, desc_head, (uint32_t)copied);

    /* record that we've used this descriptor chain now */
    vq->last_idx++;

    virtio_net_respond(dev);
    return true;
}

static virtio_device_funs_t functions = {
    .device_reset = virtio_net_reset,
    .get_device_features = virtio_net_get_device_features,
    .set_driver_features = virtio_net_set_driver_features,
    .get_device_config = virtio_net_get_device_config,
    .set_device_config = virtio_net_set_device_config,
    .queue_notify = virtio_net_queue_notify,
};

bool virtio_mmio_net_init(struct virtio_net_device *net_dev,
                     uint8_t mac[VIRTIO_NET_CONFIG_MAC_SZ],
                     uintptr_t region_base,
                     uintptr_t region_size,
                     size_t virq,
                     net_queue_handle_t *rx,
					 net_queue_handle_t *tx,
                     uintptr_t rx_data,
                     uintptr_t tx_data,
                     int rx_ch,
                     int tx_ch)
{
    struct virtio_device *dev = &net_dev->virtio_device;

    dev->data.DeviceID = DEVICE_ID_VIRTIO_SOUND;
    dev->data.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    dev->funs = &functions;
    dev->vqs = net_dev->vqs;
    dev->num_vqs = VIRTIO_NET_NUM_VIRTQ;
    dev->virq = virq;
    dev->device_data = net_dev;

    memcpy(net_dev->config.mac, mac, VIRTIO_NET_CONFIG_MAC_SZ);

    net_dev->rx = *rx;
    net_dev->tx = *tx;
    net_dev->rx_data = (void *)rx_data;
    net_dev->tx_data = (void *)tx_data;
    net_dev->rx_ch = rx_ch;
    net_dev->tx_ch = tx_ch;

    return virtio_mmio_register_device(dev, region_base, region_size, virq);
}
