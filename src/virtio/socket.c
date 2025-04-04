/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/virq.h>
#include <libvmm/util/util.h>
#include <libvmm/virtio/config.h>
#include <libvmm/virtio/mmio.h>
#include <libvmm/virtio/socket.h>
#include <sddf/util/string.h>

/* Uncomment this to enable debug logging */
// #define DEBUG_VSOCK

#if defined(DEBUG_VSOCK)
#define LOG_VSOCK(...) do{ printf("%s|VIRTIO(VSOCK): ", microkit_name); printf(__VA_ARGS__); }while(0)
#else
#define LOG_VSOCK(...) do{}while(0)
#endif

#define LOG_VSOCK_ERR(...) do{ printf("%s|VIRTIO(VSOCK)|ERROR: ", microkit_name); printf(__VA_ARGS__); }while(0)

static bool virtio_vsock_valid_cid(uint32_t cid) {
    // Page 159 of 282 VirtIO spec 1.2.
    switch (cid) {
        case 0:
        case 1:
        case 2:
        case 0xffffffff:
            return false;
        default:
            return true;
    }
}

static void virtio_vsock_features_print(uint32_t features) {
    /* Dump the features given in a human-readable format */
    LOG_VSOCK("Dumping features (0x%lx):\n", features);
    LOG_VSOCK("feature VIRTIO_VSOCK_F_STREAM set to %s\n",
             BIT_LOW(VIRTIO_VSOCK_F_STREAM) & features ? "true" : "false");
    LOG_VSOCK("feature VIRTIO_VSOCK_F_SEQPACKET set to %s\n",
             BIT_LOW(VIRTIO_VSOCK_F_SEQPACKET) & features ? "true" : "false");
}

static void virtio_vsock_reset(struct virtio_device *dev) {
    LOG_VSOCK("operation: reset\n");

    for (int i = 0; i < dev->num_vqs; i++){
        dev->vqs[i].ready = false;
        dev->vqs[i].last_idx = 0;
    }
}

static bool virtio_vsock_get_device_features(struct virtio_device *dev, uint32_t *features) {
    LOG_VSOCK("operation: get device features\n");

    switch (dev->data.DeviceFeaturesSel) {
        case 0:
            *features = BIT_LOW(VIRTIO_VSOCK_F_STREAM);
            break;
        case 1:
            *features = BIT_HIGH(VIRTIO_F_VERSION_1);
            break;
        default:
            // @ivanv: audit
            LOG_VSOCK_ERR("driver sets DeviceFeaturesSel to 0x%x, which doesn't make sense\n", dev->data.DeviceFeaturesSel);
            return false;
    }

    return true;
}

static bool virtio_vsock_set_driver_features(struct virtio_device *dev, uint32_t features) {
    LOG_VSOCK("operation: set driver features\n");
    virtio_vsock_features_print(features);

    bool success = true;

    switch (dev->data.DriverFeaturesSel) {
        // feature bits 0 to 31
        case 0:
            break;
        // features bits 32 to 63
        case 1:
            success = (features == BIT_HIGH(VIRTIO_F_VERSION_1));
            break;
        default:
            LOG_VSOCK_ERR("driver sets DriverFeaturesSel to 0x%x, which doesn't make sense\n", dev->data.DriverFeaturesSel);
            return false;
    }

    if (success) {
        dev->data.features_happy = 1;
        LOG_VSOCK("device is feature happy\n");
    }

    return true;
}

static bool virtio_vsock_get_device_config(struct virtio_device *dev, uint32_t offset, uint32_t *config) {
    LOG_VSOCK("operation: get device config at offset 0x%lx\n", offset - REG_VIRTIO_MMIO_CONFIG);

    struct virtio_vsock_device *vsock = (struct virtio_vsock_device *)dev->device_data;

    switch (offset) {
        case REG_VIRTIO_MMIO_CONFIG:
            *config = vsock->guest_cid;
            break;
        case REG_VIRTIO_MMIO_CONFIG + 0x4:
            /* The upper 32-bits of the CID are reserved and zeroed. */
            *config = 0;
            break;
        default:
            LOG_VSOCK("get config at unknown register offset 0x%x\n", offset);
            return false;
    }

    return true;
}

static bool virtio_vsock_set_device_config(struct virtio_device *dev, uint32_t offset, uint32_t config) {
    LOG_VSOCK("operation: set device config\n");
    return false;
}

static void virtio_vsock_handle_tx(struct virtio_device *dev) {
    struct virtio_vsock_device *vsock = (struct virtio_vsock_device *) dev->device_data;
    struct virtio_queue_handler *vq = &dev->vqs[VIRTIO_VSOCK_TX_QUEUE];
    struct virtio_vsock_recv_space *peer_buf = (struct virtio_vsock_recv_space *) vsock->buffer_peer;
    struct virtio_vsock_recv_space *our_buf = (struct virtio_vsock_recv_space *) vsock->buffer_our;

    /* Process 1 transmit buffer if one exists. */
    if (vq->last_idx != vq->virtq.avail->idx) {
        /* Before doing anything, check to make sure the other side can actually receive data. */
        if (peer_buf->metadata.dirty) {
            return;
        }

        uint16_t desc_idx = vq->virtq.avail->ring[vq->last_idx % vq->virtq.num];
        struct virtq_desc desc;
        desc = vq->virtq.desc[desc_idx];
        LOG_VSOCK("======== TRANSMITTING ========\n");

        struct virtio_vsock_packet *packet = (struct virtio_vsock_packet *) desc.addr;
        LOG_VSOCK("src_cid: 0x%lx\n", packet->hdr.src_cid);
        LOG_VSOCK("dst_cid: 0x%lx\n", packet->hdr.dst_cid);
        LOG_VSOCK("src_port: 0x%lx\n", packet->hdr.src_port);
        LOG_VSOCK("dst_port: 0x%lx\n", packet->hdr.dst_port);
        LOG_VSOCK("len: 0x%lx\n", packet->hdr.len);
        LOG_VSOCK("type: 0x%lx\n", packet->hdr.type);
        LOG_VSOCK("op: 0x%lx\n", packet->hdr.op);
        LOG_VSOCK("flags: 0x%lx\n", packet->hdr.flags);
        LOG_VSOCK("buf_alloc: 0x%lx\n", packet->hdr.buf_alloc);
        LOG_VSOCK("fwd_cnt: 0x%lx\n", packet->hdr.fwd_cnt);

        /* We only support VIRTIO_VSOCK_TYPE_STREAM */
        assert(packet->hdr.type == VIRTIO_VSOCK_TYPE_STREAM);

        /* Check the guest (src) and dest CIDs are valid */
        assert(virtio_vsock_valid_cid(packet->hdr.src_cid));
        assert(virtio_vsock_valid_cid(packet->hdr.dst_cid));
        assert(packet->hdr.src_cid == vsock->guest_cid);

        switch (packet->hdr.op) {
            case VIRTIO_VSOCK_OP_REQUEST:
            case VIRTIO_VSOCK_OP_RESPONSE:
            case VIRTIO_VSOCK_OP_RST:
            case VIRTIO_VSOCK_OP_SHUTDOWN:
            case VIRTIO_VSOCK_OP_CREDIT_UPDATE:
            case VIRTIO_VSOCK_OP_CREDIT_REQUEST:
                /* These requests don't have any payloads. */
                assert(packet->hdr.len == 0);
                break;
            case VIRTIO_VSOCK_OP_RW: {
                /* Only this one have a payload because it actually sends data */
                /* Sanity check that the payload does not exceed the device's buffer. Shouldn't
                    trip as we've previously informed the other side of our real buffer size. */
                assert(packet->hdr.len <= vsock->payload_size);
                break;
            }
            default: {
                LOG_VSOCK_ERR("invalid operation %d\n", packet->hdr.op);
                // TODO: handle
                // send reset as per spec
                // 5.10.6.4.2
                assert(false);
                break;
            }
        }

        /* Copy the head to the receiver's RX buffer. */
        memcpy(&peer_buf->packet.hdr, &packet->hdr, sizeof(struct virtio_vsock_hdr));

        /* Then doctor the header to tell the receiver the size of our device receive buffer */
        peer_buf->packet.hdr.buf_alloc = vsock->payload_size;
        
        /* Copy the payload if the request is a send */
        if (packet->hdr.op == VIRTIO_VSOCK_OP_RW) {
            uintptr_t payload = (uintptr_t) &packet->data;
            if (vq->virtq.desc[desc_idx].flags & VIRTQ_DESC_F_NEXT) {
                /* Linux tends to put the payload in a separate descriptor for zero-copy TX so we
                   need to handle accordingly. */
                payload = vq->virtq.desc[desc.next].addr;
                assert(vq->virtq.desc[desc.next].len == packet->hdr.len);
            } else {
                assert(vq->virtq.desc[desc_idx].len == sizeof(struct virtio_vsock_hdr) + packet->hdr.len);
            }
            memcpy(&peer_buf->packet.data, (void *) payload, packet->hdr.len);
        }

        /* Prevent further TX to the receiver until they have consumed this packet */
        peer_buf->metadata.dirty = true;

        struct virtq_used_elem used_hdr_elem = {desc_idx, sizeof(struct virtio_vsock_hdr) + packet->hdr.len};
        vq->virtq.used->ring[vq->virtq.used->idx % vq->virtq.num] = used_hdr_elem;
        vq->virtq.used->idx++;
        vq->last_idx++;

        if (vq->last_idx != vq->virtq.avail->idx) {
            /* Still got packets to send, but out of buffer for now. */
            LOG_VSOCK("=> Sender requesting signal\n");
            our_buf->metadata.signal_required = true;
        } else {
            LOG_VSOCK("=> Sender processed all TX\n");
            our_buf->metadata.signal_required = false;
        }

        microkit_notify(vsock->peer_ch);
    }
}

bool virtio_vsock_handle_rx(struct virtio_vsock_device *vsock) {
    struct virtio_device *dev = &vsock->virtio_device;
    struct virtio_vsock_recv_space *our_buf = (struct virtio_vsock_recv_space *) vsock->buffer_our;
    struct virtio_vsock_recv_space *peer_buf = (struct virtio_vsock_recv_space *) vsock->buffer_peer;

    if (our_buf->metadata.signal_required) {
        /* If our signal required bit is set, it means that previously we have packets to send while the
           receiver's buffer is dirty. Now that we got notified, it means that the receiver is ready for
           our next packet. Send it! */
        virtio_vsock_handle_tx(dev);
    } else {
        /* Normal receive */
        struct virtio_queue_handler *vq = &dev->vqs[VIRTIO_VSOCK_RX_QUEUE];
        struct virtio_vsock_packet *packet = (struct virtio_vsock_packet *) &our_buf->packet;
    
        LOG_VSOCK("======== RECEIVING ========\n");
        LOG_VSOCK("src_cid: 0x%lx\n", packet->hdr.src_cid);
        LOG_VSOCK("dst_cid: 0x%lx\n", packet->hdr.dst_cid);
        LOG_VSOCK("src_port: 0x%lx\n", packet->hdr.src_port);
        LOG_VSOCK("dst_port: 0x%lx\n", packet->hdr.dst_port);
        LOG_VSOCK("len: 0x%lx\n", packet->hdr.len);
        LOG_VSOCK("type: 0x%lx\n", packet->hdr.type);
        LOG_VSOCK("op: 0x%lx\n", packet->hdr.op);
        LOG_VSOCK("flags: 0x%lx\n", packet->hdr.flags);
        LOG_VSOCK("buf_alloc: 0x%lx\n", packet->hdr.buf_alloc);
        LOG_VSOCK("fwd_cnt: 0x%lx\n", packet->hdr.fwd_cnt);
    
        /* Grab an available descriptor and copy over the packet's header */
        uint16_t desc_head = vq->virtq.avail->ring[vq->last_idx % vq->virtq.num];
        struct virtq_desc desc = vq->virtq.desc[desc_head];
        memcpy((void *) desc.addr, &packet->hdr, sizeof(struct virtio_vsock_hdr));
    
        if (packet->hdr.op == VIRTIO_VSOCK_OP_RW) {
            void *payload_dest = (void *)((uintptr_t) desc.addr + sizeof(struct virtio_vsock_hdr));
            /* Some version of Linux likes to chain RX descriptor for zero-copy RX, we have to handle that
               in the driver as well. */
            if (desc.flags & VIRTQ_DESC_F_NEXT) {
                payload_dest = (void *) vq->virtq.desc[desc.next].addr;
                assert(vq->virtq.desc[desc.next].len >= packet->hdr.len);
            } else {
                assert(desc.len >= sizeof(struct virtio_vsock_hdr) + packet->hdr.len);
            }
            memcpy(payload_dest, packet->data, packet->hdr.len);
        }
    
        struct virtq_used_elem used_hdr_elem = {desc_head, sizeof(struct virtio_vsock_hdr) + packet->hdr.len};
        vq->virtq.used->ring[vq->virtq.used->idx % vq->virtq.num] = used_hdr_elem;
        vq->virtq.used->idx++;
        vq->last_idx++;
    
        /* Initiate to our peer that we are ready to accept the next packet. */
        our_buf->metadata.dirty = false;
    
        /* The peer might have stopped sending because our buffer was dirty,
           or we might have stopped sending because the peer buffer was dirty
           check if we have anything to send and notify them that we are ready
           to receive the next packet. */
        if (peer_buf->metadata.signal_required) {
            LOG_VSOCK("=> Peer requested signal\n");
            microkit_notify(vsock->peer_ch);
        }

        /* Inject an IRQ to tell the guest that a packet has been received. */
        dev->data.InterruptStatus = BIT_LOW(0);
        LOG_VSOCK("operation: injecting virq %d\n", dev->virq);
        bool success = virq_inject(GUEST_VCPU_ID, dev->virq);
        assert(success);
    }

    return true;
}

static bool virtio_vsock_handle_queue_notify(struct virtio_device *dev) {
    LOG_VSOCK("operation: handle queue_notify on %d\n", dev->data.QueueNotify);

    size_t vq_idx = dev->data.QueueNotify;
    if (vq_idx >= dev->num_vqs) {
        LOG_VSOCK_ERR("invalid virtq index %d\n", vq_idx);
        return false;
    }

    if (vq_idx != VIRTIO_VSOCK_TX_QUEUE) return true;
    virtio_vsock_handle_tx(dev);

    /* As of virtIO v1.2, the event vq isn't important in our device implementation
       because it is used to account for guest migrating to a different host. */

    return true;
}

static virtio_device_funs_t functions = {
    .device_reset = virtio_vsock_reset,
    .get_device_features = virtio_vsock_get_device_features,
    .set_driver_features = virtio_vsock_set_driver_features,
    .get_device_config = virtio_vsock_get_device_config,
    .set_device_config = virtio_vsock_set_device_config,
    .queue_notify = virtio_vsock_handle_queue_notify,
};

bool virtio_mmio_vsock_init(struct virtio_vsock_device *vsock,
                            uintptr_t region_base,
                            uintptr_t region_size,
                            size_t virq,
                            uint32_t guest_cid,
                            uint32_t shared_buffer_size,
                            uintptr_t buffer_our,
                            uintptr_t buffer_peer,
                            microkit_channel peer_ch)
{
    assert(shared_buffer_size >= 0x1000);
    assert(buffer_our != buffer_peer);

    /* First check whether or not we have a valid guest CID */
    if (!virtio_vsock_valid_cid(guest_cid)) {
        LOG_VSOCK_ERR("attempted to init vsock device with invalid guest CID %d\n", guest_cid);
        return false;
    }
    LOG_VSOCK("registering vsock device with cid %u\n", guest_cid);

    struct virtio_device *dev = &vsock->virtio_device;
    dev->data.DeviceID = VIRTIO_DEVICE_ID_SOCKET;
    dev->data.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    dev->funs = &functions;
    dev->vqs = vsock->vqs;
    dev->num_vqs = VIRTIO_VSOCK_NUM_VIRTQ;
    dev->virq = virq;
    dev->device_data = vsock;

    vsock->guest_cid = guest_cid;
    vsock->shared_buffer_size = shared_buffer_size;
    vsock->payload_size = shared_buffer_size - sizeof(struct virtio_vsock_recv_space_metadata) - sizeof(struct virtio_vsock_hdr);
    vsock->buffer_our = buffer_our;
    vsock->buffer_peer = buffer_peer;
    vsock->peer_ch = peer_ch;

    struct virtio_vsock_recv_space *our_buf = (struct virtio_vsock_recv_space *) vsock->buffer_our;
    our_buf->metadata.dirty = false;
    our_buf->metadata.signal_required = false;

    if (!virtio_mmio_register_device(dev, region_base, region_size, virq)) {
        LOG_VSOCK_ERR("Couldn't register mmio device in init().");
        return false;
    }

    return true;
}
