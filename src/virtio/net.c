#include "virtio/config.h"
#include "virtio/mmio.h"
#include "virtio/net.h"
#include "virtio/net_config.h"
#include "util/util.h"
#include "virq.h"

// @ivanv: put in util or remove
#define BIT_LOW(n)  (1ul<<(n))
#define BIT_HIGH(n) (1ul<<(n - 32 ))

/* Uncomment this to enable debug logging */
// #define DEBUG_CONSOLE

#if defined(DEBUG_NET)
#define LOG_NET(...) do{ printf("VIRTIO(NET): "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_NET(...) do{}while(0)
#endif

#define LOG_NET_ERR(...) do{ printf("VIRTIO(NET)|ERROR: "); printf(__VA_ARGS__); }while(0)
#define LOG_NET_WARNING(...) do{ printf("VIRTIO(NET)|WARNING: "); printf(__VA_ARGS__); }while(0)


// @jade: I don't know why we need these but the driver seems to care
typedef enum {
    ORIGIN_RX_QUEUE,
    ORIGIN_TX_QUEUE,
} ethernet_buffer_origin_t;

// temporary buffer to transmit buffer from this layer to the backend layer
// @jade: get rid of this
static char temp_buf[TMP_BUF_SIZE];

#define NUM_SDDF_CHANNEL    2
static size_t channels[NUM_SDDF_CHANNEL];

static bool virtio_net_mmio_handle_rx(void *buf, uint32_t size);

static void dump_packet(int len, uint8_t *buffer, bool incoming)
{
    if (incoming) {
        printf("\"%s\"|VIRTIO MMIO|INFO: incoming, dest MAC: "PR_MAC802_ADDR", src MAC: "PR_MAC802_ADDR", type: 0x%02x%02x\n",
                microkit_name, PR_MAC802_DEST_ADDR_ARGS(macaddr), PR_MAC802_SRC_ADDR_ARGS(macaddr),
                macaddr->etype[0], macaddr->etype[1]);
    } else {
        printf("\"%s\"|VIRTIO MMIO|INFO: outgoing, dest MAC: "PR_MAC802_ADDR", src MAC: "PR_MAC802_ADDR", type: 0x%02x%02x\n",
            microkit_name, PR_MAC802_DEST_ADDR_ARGS(macaddr), PR_MAC802_SRC_ADDR_ARGS(macaddr),
            macaddr->etype[0], macaddr->etype[1]);
    }
    printf("-------------------- payload start --------------------\n");
    for (int i = 0; i < len; i++) {
        printf("%02x ", buffer[i]);
    }
    printf("\n");
    printf("--------------------- payload end ---------------------\n");
}

static void net_client_get_mac(uint8_t *retval)
{
    microkit_ppcall(NET_CLIENT_GET_MAC_CH, microkit_msginfo_new(0, 0));
    uint32_t palr = microkit_mr_get(0);
    uint32_t paur = microkit_mr_get(1);

    retval[5] = paur >> 8 & 0xff;
    retval[4] = paur & 0xff;
    retval[3] = palr >> 24;
    retval[2] = palr >> 16 & 0xff;
    retval[1] = palr >> 8 & 0xff;
    retval[0] = palr & 0xff;
    // printf("\"%s\"|VMM NET CLIENT|INFO: net_client_get_mac\n", microkit_name);
}

// sent packet from this vmm to another
static bool net_client_tx(void *buf, uint32_t size)
{
    uintptr_t addr;
    unsigned int len;
    void *cookie;

    // get a buffer from the free ring
    int error = dequeue_free(&net_client_tx_ring, &addr, &len, &cookie);
    if (error) {
        printf("\"%s\"|VMM NET CLIENT|INFO: free ring is empty\n", microkit_name);
        return false;
    }
    assert(size <= len);

    // @jade: eliminate this copy
    memcpy((void *)addr, buf, size);

    struct ether_addr *macaddr = (struct ether_addr *)addr;
    // if (macaddr->etype[0] & 0x8) {
        // printf("\"%s\"|VIRTIO MMIO|INFO: outgoing, dest MAC: "PR_MAC802_ADDR", src MAC: "PR_MAC802_ADDR", type: 0x%02x%02x\n",
        //         microkit_name, PR_MAC802_DEST_ADDR_ARGS(macaddr), PR_MAC802_SRC_ADDR_ARGS(macaddr),
        //         macaddr->etype[0], macaddr->etype[1]);
        // dump_payload(size - 14, macaddr->payload);
    // }
    /* insert into the used ring */
    error = enqueue_used(&net_client_tx_ring, addr, size, NULL);
    if (error) {
        printf("\"%s\"|VMM NET CLIENT|INFO: TX used ring full\n", microkit_name);
        enqueue_free(&net_client_tx_ring, addr, len, NULL);
        return false;
    }

    /* notify the other end */
    microkit_notify(NET_CLIENT_TX_CH);

    return true;
}

// @jade: bad OOP design. This function should not know about dev
bool net_client_rx(struct virtio_device *dev)
{
    uintptr_t addr;
    unsigned int len;
    void *cookie;

    while(!ring_empty(net_client_rx_ring.used_ring)) {
        int error = dequeue_used(&net_client_rx_ring, &addr, &len, &cookie);
        // RX used ring is empty, this is not suppose to happend!
        assert(!error);

        // struct ether_addr *macaddr = (struct ether_addr *)addr;
        // if (macaddr->etype[0] & 0x8 && macaddr->etype[1] & 0x6) {
        //     printf("\"%s\"|VIRTIO MMIO|INFO: incoming, dest MAC: "PR_MAC802_ADDR", src MAC: "PR_MAC802_ADDR", type: 0x%02x%02x\n",
        //             microkit_name, PR_MAC802_DEST_ADDR_ARGS(macaddr), PR_MAC802_SRC_ADDR_ARGS(macaddr),
        //             macaddr->etype[0], macaddr->etype[1]);
        //     dump_payload(len - 14, macaddr->payload);
        // }
        // @jade: handle errors
        virtio_net_handle_rx((void *)addr, len);
        enqueue_free(&net_client_rx_ring, addr, SHMEM_BUF_SIZE, NULL);
    }

    return true;
}

void virtio_net_reset(struct virtio_device *dev)
{
    LOG_NET("operation: reset\n");
    dev->vqs[RX_QUEUE].ready = 0;
    dev->vqs[RX_QUEUE].last_idx = 0;

    dev->vqs[TX_QUEUE].ready = 0;
    dev->vqs[TX_QUEUE].last_idx = 0;
}


int virtio_net_get_device_features(struct virtio_device *dev, uint32_t *features)
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
            LOG_NET_WARNING("driver sets DeviceFeaturesSel to 0x%x, which doesn't make sense\n", dev->data.DeviceFeaturesSel);
            return 0;
    }
    return 1;
}


int virtio_net_set_driver_features(struct virtio_device *dev, uint32_t features)
{
    LOG_NET("operation: set device features\n");
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
            LOG_NET_WARNING("driver sets DriverFeaturesSel to 0x%x, which doesn't make sense\n", dev->data.DriverFeaturesSel);
            success = 0;
    }
    if (success) {
        dev->data.features_happy = 1;
        LOG_NET("the virtio driver is happy about the features\n");
    }
    return success;
}

int virtio_net_get_device_config(struct virtio_device *dev, uint32_t offset, uint32_t *ret_val)
{
    // @jade: this function might need a refactor when the virtio net backend starts to
    // support more features
    switch (offset) {
        // get mac low
        case REG_RANGE(0x100, 0x104):
        {
            uint8_t mac[6];
            net_client_get_mac(mac);
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
            net_client_get_mac(mac);
            *ret_val = mac[4];
            *ret_val |= mac[5] << 8;
            break;
        }
        default:
            LOG_NET_WARNING("unknown device config register: 0x%x\n", offset);
            return 0;
    }
    return -1;
}

int virtio_net_set_device_config(uint32_t offset, uint32_t val)
{
    LOG_NET_WARNING("driver attempts to set device config but virtio net only has driver-read-only configuration fields\n");
    return -1;
}

// handle queue notify from the guest VM
static int virtio_net_handle_tx(struct virtio_device *dev)
{
    LOG_NET("operation: handle transmit\n");
    assert(dev->num_vqs > TX_QUEUE);

    struct virtq *virtq = &dev->vqs[TX_QUEUE].virtq;

    /* read the current guest index */
    uint16_t guest_idx = virtq->avail->idx;
    uint16_t idx = &dev->vqs[TX_QUEUE].last_idx;

    for (; idx != guest_idx; idx++) {
        /* read the head of the descriptor chain */
        uint16_t desc_head = virtq->avail->ring[idx % virtq->num];

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
                skipping = MIN(sizeof(struct virtio_net_hdr_mrg_rxbuf) - skipped, virtq->desc[curr_desc_head].len);
                skipped += skipping;
            }

            /* truncate packets that are large than BUF_SIZE */
            uint32_t writing = MIN(TMP_BUF_SIZE - written, virtq->desc[curr_desc_head].len - skipping);

            // @jade: we want to eliminate this copy
            memcpy(temp_buf + written, (void *)virtq->desc[curr_desc_head].addr + skipping, writing);
            written += writing;
            curr_desc_head = virtq->desc[curr_desc_head].next;
        } while (virtq->desc[curr_desc_head].flags & VRING_DESC_F_NEXT);

        /* ship the buffer to the next layer */
        int success = net_client_tx(temp_buf, written);
        if (!success) {
            LOG_NET_ERR("failed to deliver packet for the guest\n.");
        }

        //add to useds
        struct virtq_used_elem used_elem = {desc_head, 0};
        uint16_t guest_idx = virtq->used->idx;

        virtq->used->ring[guest_idx % virtq->num] = used_elem;
        virtq->used->idx++;
    }

    dev->vqs[TX_QUEUE].last_idx = idx;

    // set the reason of the irq
    dev->data.InterruptStatus = BIT_LOW(0);

    // notify the guest VM that we successfully delivered their packet(s)
    bool success = virq_inject(GUEST_VCPU_ID, VIRTIO_NET_IRQ);
    // we can't inject irqs?? panic.
    assert(success);

    return success;
}

// handle rx from client
static bool virtio_net_handle_rx(struct virtio_device *dev, void *buf, uint32_t size)
{
    if (!dev->vqs[RX_QUEUE].ready) {
        // vq is not initialised, drop the packet
        return false;
    }
    struct virtq *virtq = &dev->vqs[RX_QUEUE].virtq;

    /* grab the next receive chain */
    uint16_t guest_idx = virtq->avail->idx;
    uint16_t idx = dev->vqs[RX_QUEUE].last_idx;

    if (idx == guest_idx) {
        // vq is full or not fully initialised (in this case idx and guest_idx are both 0s), drop the packet
        return false;
    }

    struct virtio_net_hdr_mrg_rxbuf virtio_hdr = {0};
    virtio_hdr.num_buffers = 1;
    // memzero(&virtio_hdr, sizeof(virtio_hdr));

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
            if (!(virtq->desc[curr_desc_head].flags & VRING_DESC_F_NEXT)) {
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
    struct virtq_used_elem used_elem = {desc_head, copied};
    uint16_t used_idx = virtq->used->idx;

    virtq->used->ring[used_idx % virtq->num] = used_elem;
    virtq->used->idx++;

    /* record that we've used this descriptor chain now */
    dev->vqs[RX_QUEUE].last_idx++;

    // set the reason of the irq
    dev->data.InterruptStatus = BIT_LOW(0);

    // notify the guest
    bool success = virq_inject(VCPU_ID, VIRTIO_NET_IRQ);
    // we can't inject irqs?? panic.
    assert(success);

    return true;
}

static virtio_device_funs_t functions = {
    .device_reset = virtio_net_mmio_reset,
    .get_device_features = virtio_net_mmio_get_device_features,
    .set_driver_features = virtio_net_mmio_set_driver_features,
    .get_device_config = virtio_net_mmio_get_device_config,
    .set_device_config = virtio_net_mmio_set_device_config,
    .queue_notify = virtio_net_mmio_handle_queue_notify_tx,
};

void virtio_net_init(struct virtio_device *dev,
                     struct virtio_queue_handler *vqs, size_t num_vqs,
                     size_t virq,
                     ring_handle_t *sddf_rx_ring, ring_handle_t *sddf_tx_ring,
                     size_t sddf_mux_tx_ch, size_t addf_mux_get_mac_ch) {
    // @ivanv: check that num_vqs is greater than the minimum vqs to function?
    dev->data.DeviceID = DEVICE_ID_VIRTIO_NET;
    dev->data.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    dev->funs = &functions;
    dev->vqs = vqs;
    dev->num_vqs = num_vqs;
    dev->virq = virq;
    dev->sddf_rx_ring = sddf_rx_ring;
    dev->sddf_tx_ring = sddf_tx_ring;

    channels[0] = sddf_mux_tx_ch;
    channels[1] = sddf_mux_get_mac_ch;
    dev->sddf_chs = channels;
}