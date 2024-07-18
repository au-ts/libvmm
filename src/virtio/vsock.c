#include <libvmm/util/util.h>
#include <libvmm/virtio/config.h>
#include <libvmm/virtio/mmio.h>
#include <libvmm/virtio/vsock.h>

/* Uncomment this to enable debug logging */
#define DEBUG_VSOCK

#if defined(DEBUG_VSOCK)
#define LOG_VSOCK(...) do{ printf("%s|VIRTIO(VSOCK): ", microkit_name); printf(__VA_ARGS__); }while(0)
#else
#define LOG_VSOCK(...) do{}while(0)
#endif

#define LOG_VSOCK_ERR(...) do{ printf("%s|VIRTIO(VSOCK)|ERROR: ", microkit_name); printf(__VA_ARGS__); }while(0)

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

static int virtio_vsock_get_device_features(struct virtio_device *dev, uint32_t *features) {
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
            return 0;
    }

    return 1;
}

static int virtio_vsock_set_driver_features(struct virtio_device *dev, uint32_t features) {
    LOG_VSOCK("operation: set driver features\n");
    virtio_vsock_features_print(features);

    int success = 1;

    switch (dev->data.DriverFeaturesSel) {
        // feature bits 0 to 31
        case 0:
            // The device initialisation protocol says the driver should read device feature bits,
            // and write the subset of feature bits understood by the OS and driver to the device.
            // Currently we only have one feature to check.
            // success = (features == BIT_LOW(VIRTIO_NET_F_MAC));
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

    return success;
}

static int virtio_vsock_get_device_config(struct virtio_device *dev, uint32_t offset, uint32_t *config) {
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

static int virtio_vsock_set_device_config(struct virtio_device *dev, uint32_t offset, uint32_t config) {
    LOG_VSOCK("operation: set device config\n");
    return false;
}

static int virtio_vsock_handle_queue_notify(struct virtio_device *dev) {
    LOG_VSOCK("operation: handle queue_notify on %d\n", dev->data.QueueNotify);

    size_t vq_idx = dev->data.QueueNotify;
    if (vq_idx >= dev->num_vqs) {
        LOG_VSOCK_ERR("invalid virtq index %d\n", vq_idx);
        return false;
    }

    struct virtio_queue_handler *vq = &dev->vqs[vq_idx];
    LOG_VSOCK("processing available buffers from index [0x%lx..0x%lx)\n", vq->last_idx, vq->virtq.avail->idx);

    if (vq_idx != VIRTIO_VSOCK_TX_QUEUE) return true;

    /* Got something in the transmit queue, process it */
    while (vq->last_idx != vq->virtq.avail->idx) {
        uint16_t desc_idx = vq->virtq.avail->ring[vq->last_idx % vq->virtq.num];
        struct virtq_desc desc;
        do {
            desc = vq->virtq.desc[desc_idx];
            LOG_VSOCK("processing descriptor (0x%lx) with buffer [0x%lx..0x%lx)\n", desc_idx, desc.addr, desc.addr + desc.len);
            // @ivanv: need to check that the guest-phys addr is sane so we don't crash the VMM!

            struct virtio_vsock_hdr *hdr = (struct virtio_vsock_hdr *) desc.addr;

            LOG_VSOCK("src_cid: 0x%lx\n", hdr->src_cid);
            LOG_VSOCK("dst_cid: 0x%lx\n", hdr->dst_cid);
            LOG_VSOCK("src_port: 0x%lx\n", hdr->src_port);
            LOG_VSOCK("dst_port: 0x%lx\n", hdr->dst_port);
            LOG_VSOCK("len: 0x%lx\n", hdr->len);
            LOG_VSOCK("type: 0x%lx\n", hdr->type);
            LOG_VSOCK("op: 0x%lx\n", hdr->op);
            LOG_VSOCK("flags: 0x%lx\n", hdr->flags);
            LOG_VSOCK("buf_alloc: 0x%lx\n", hdr->buf_alloc);
            LOG_VSOCK("fwd_cnt: 0x%lx\n", hdr->fwd_cnt);

            desc_idx = desc.next;
        } while (desc.flags & VIRTQ_DESC_F_NEXT);

        vq->last_idx++;
    }

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
                         uint32_t guest_cid)
{
    /* First check whether or not we have a valid guest CID */
    if (guest_cid <= 2 || guest_cid == 0xffffffff) {
        LOG_VSOCK_ERR("attempted to init vsock device with invalid guest CID %d\n", guest_cid);
        return false;
    }

    struct virtio_device *dev = &vsock->virtio_device;
    dev->data.DeviceID = DEVICE_ID_VIRTIO_VSOCK;
    dev->data.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    dev->funs = &functions;
    dev->vqs = vsock->vqs;
    dev->num_vqs = VIRTIO_VSOCK_NUM_VIRTQ;
    dev->virq = virq;
    dev->device_data = vsock;

    vsock->guest_cid = guest_cid;

    return virtio_mmio_register_device(dev, region_base, region_size, virq);
}
