/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/guest.h>
#include <libvmm/virq.h>
#include <libvmm/util/util.h>
#include <libvmm/virtio/config.h>
#include <libvmm/virtio/mmio.h>
#include <libvmm/virtio/console.h>
#include <libvmm/virtio/virtio.h>
#include <sddf/serial/queue.h>

/* Uncomment this to enable debug logging */
// #define DEBUG_CONSOLE

#if defined(DEBUG_CONSOLE)
#define LOG_CONSOLE(...) do{ printf("VIRTIO(CONSOLE): "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_CONSOLE(...) do{}while(0)
#endif

#define LOG_CONSOLE_ERR(...) do{ printf("VIRTIO(CONSOLE)|ERROR: "); printf(__VA_ARGS__); }while(0)

static inline struct virtio_console_device *device_state(struct virtio_device *dev)
{
    return (struct virtio_console_device *)dev->device_data;
}

static void virtio_console_features_print(uint32_t features)
{
    /* Dump the features given in a human-readable format */
    LOG_CONSOLE("Dumping features (0x%lx):\n", features);
    LOG_CONSOLE("feature VIRTIO_CONSOLE_F_SIZE set to %s\n",
                BIT_LOW(VIRTIO_CONSOLE_F_SIZE) & features ? "true" : "false");
    LOG_CONSOLE("feature VIRTIO_CONSOLE_F_MULTIPORT set to %s\n",
                BIT_LOW(VIRTIO_CONSOLE_F_MULTIPORT) & features ? "true" : "false");
    LOG_CONSOLE("feature VIRTIO_CONSOLE_F_EMERG_WRITE set to %s\n",
                BIT_LOW(VIRTIO_CONSOLE_F_EMERG_WRITE) & features ? "true" : "false");
}

static void virtio_console_reset(struct virtio_device *dev)
{
    LOG_CONSOLE("operation: reset device\n");

    for (int i = 0; i < dev->num_vqs; i++) {
        dev->vqs[i].ready = false;
        dev->vqs[i].last_idx = 0;
    }
}

static bool virtio_console_get_device_features(struct virtio_device *dev, uint32_t *features)
{
    LOG_CONSOLE("operation: get device features\n");

    switch (dev->regs.DeviceFeaturesSel) {
    case 0:
        *features = 0;
        break;
    case 1:
        *features = BIT_HIGH(VIRTIO_F_VERSION_1);
        break;
    default:
        LOG_CONSOLE_ERR("driver sets DeviceFeaturesSel to 0x%x, which doesn't make sense\n",
                        dev->regs.DeviceFeaturesSel);
        return false;
    }

    return true;
}

static bool virtio_console_set_driver_features(struct virtio_device *dev, uint32_t features)
{
    LOG_CONSOLE("operation: set driver features\n");
    virtio_console_features_print(features);

    bool success = false;

    switch (dev->regs.DriverFeaturesSel) {
    // feature bits 0 to 31
    case 0:
        /* We do not offer any features in the first 32-bit bits */
        success = (features == 0);
        break;
    // features bits 32 to 63
    case 1:
        success = (features == BIT_HIGH(VIRTIO_F_VERSION_1));
        break;
    default:
        LOG_CONSOLE_ERR("driver sets DriverFeaturesSel to 0x%x, which doesn't make sense\n",
                        dev->regs.DriverFeaturesSel);
        return false;
    }

    if (success) {
        dev->features_happy = 1;
        LOG_CONSOLE("device is feature happy\n");
    }

    return success;
}

static bool virtio_console_get_device_config(struct virtio_device *dev, uint32_t offset, uint32_t *config)
{
    LOG_CONSOLE("operation: get device config\n");
    return false;
}

static bool virtio_console_set_device_config(struct virtio_device *dev, uint32_t offset, uint32_t config)
{
    LOG_CONSOLE("operation: set device config\n");
    return false;
}

static bool virtio_console_handle_tx(struct virtio_device *dev)
{
    LOG_CONSOLE("operation: handle transmit\n");
    // @ivanv: we need to check the pre-conditions before doing anything. e.g check
    // TX_QUEUE is ready?
    assert(dev->num_vqs > TX_QUEUE);
    struct virtio_queue_handler *vq = &dev->vqs[TX_QUEUE];
    struct virtio_console_device *console = device_state(dev);

    /* Transmit all available descriptors possible */
    LOG_CONSOLE("processing available buffers from index [0x%lx..0x%lx)\n", vq->last_idx, vq->virtq.avail->idx);
    bool transferred = false;
    uint16_t desc_head;
    while (virtio_virtq_peek_avail(vq, &desc_head) && !serial_queue_full(console->txq, console->txq->queue->head)) {
        uint64_t payload_len = virtio_desc_chain_payload_len(vq, desc_head);

        if (payload_len > console->txq->capacity) {
            // @billn, fix properly by partial TX, bookkeep, then continue once serial virt notifies?
            LOG_CONSOLE_ERR(
                "payload length 0x%lx bytes of desciptor %u is larger than serial TX queue capacity of 0x%x bytes\n",
                payload_len, desc_head, console->txq->capacity);
            assert(false);
        }

        uint32_t serial_txq_free_len = serial_queue_free(console->txq);

        if (payload_len > serial_txq_free_len) {
            /* Can't consume this descriptor in full for now, just wait for serial TXQ to drain. */
            LOG_CONSOLE("payload length 0x%lx bytes of desciptor %u is larger than serial TX queue contiguous free "
                        "length of 0x%lx bytes, won't consume for now.\n",
                        payload_len, desc_head, serial_txq_free_len);
            serial_request_consumer_signal(console->txq);
            break;
        }
        /* Ok there is enough space in TX queue for this descriptor */

        uint32_t serial_txq_contiguous_free_len = serial_queue_contiguous_free(console->txq);
        bool copy_twice = false;
        if (payload_len > serial_txq_contiguous_free_len) {
            /* Handle case where the free space wraps around */
            copy_twice = true;
        }

        uint32_t bytes_copied = 0;

        /* Copy data until no more to copy or until the queue wraps around */
        char *serial_txq_dest = (char *)(console->txq->data_region
                                         + (console->txq->queue->tail % console->txq->capacity));
        uint32_t copy_len = MIN(payload_len, serial_txq_contiguous_free_len);
        assert(virtio_read_data_from_desc_chain(vq, desc_head, copy_len, bytes_copied, serial_txq_dest));
        bytes_copied += copy_len;
        serial_update_shared_tail(console->txq, console->txq->queue->tail + copy_len);

        if (copy_twice) {
            /* Need to copy more data after the queue wraps around */
            serial_txq_dest = (char *)(console->txq->data_region
                                       + (console->txq->queue->tail % console->txq->capacity));
            copy_len = payload_len - bytes_copied;
            assert(copy_len <= serial_queue_contiguous_free(console->txq));
            assert(virtio_read_data_from_desc_chain(vq, desc_head, copy_len, bytes_copied, serial_txq_dest));
            bytes_copied += copy_len;
            serial_update_shared_tail(console->txq, console->txq->queue->tail + copy_len);
        }

        assert(bytes_copied == payload_len);

        LOG_CONSOLE("processed descriptor %u with content: %s\n", desc_head, serial_txq_dest);

        virtio_virtq_add_used(vq, desc_head, 0);
        virtio_virtq_pop_avail(vq, &desc_head);
        transferred = true;
    }

    /* While unlikely, it is possible that we could not consume any of the
     * available data. In this case we do not set the IRQ status. */
    if (transferred) {
        dev->regs.InterruptStatus = BIT_LOW(0);
        bool success = virq_inject(dev->virq);
        assert(success);

        microkit_notify(console->tx_ch);

        return success;
    }

    return true;
}

bool virtio_console_handle_rx(struct virtio_console_device *console)
{
    LOG_CONSOLE("operation: handle rx\n");
    assert(console->virtio_device.num_vqs > RX_QUEUE);

    /* Previously if a TX request have payload length:
     * TX queue free length < payload length <= TX queue capacity
     * We request the serial virtualiser to notify us once it has consumed data from the TX queue.
     * So try to reprocess such requests. */
    virtio_console_handle_tx(&(console->virtio_device));

    /* Used to know whether to set the IRQ status. */
    bool transferred = false;

    struct virtio_queue_handler *vq = &console->virtio_device.vqs[RX_QUEUE];
    if (!vq->ready) {
        /*
         * It is valid for RX from the real device before the guest has
         * started, so just dequeue all data and early return.
         */
        char c;
        while (serial_dequeue(console->rxq, &c));
        return true;
    }

    LOG_CONSOLE("processing available buffers from index [0x%lx..0x%lx)\n", vq->last_idx, vq->virtq.avail->idx);
    uint16_t desc_head;
    while (!serial_queue_empty(console->rxq, console->rxq->queue->head) && virtio_virtq_pop_avail(vq, &desc_head)) {
        transferred = true;
        struct virtq_desc desc = vq->virtq.desc[desc_head];
        LOG_CONSOLE("processing descriptor (0x%lx) with buffer [0x%lx..0x%lx)\n", desc_head, desc.addr,
                    desc.addr + desc.len);
        uint32_t bytes_written = 0;
        char c;
        while (bytes_written < desc.len && !serial_dequeue(console->rxq, &c)) {
            assert(virtio_write_data_to_desc_chain(vq, desc_head, 1, bytes_written, &c));
            bytes_written++;
        }

        virtio_virtq_add_used(vq, desc_head, bytes_written);
    }

    /* While unlikely, it is possible that we could not consume any of the
     * available data. In this case we do not set the IRQ status. */
    if (transferred) {
        console->virtio_device.regs.InterruptStatus = BIT_LOW(0);
        bool success = virq_inject(console->virtio_device.virq);
        assert(success);

        return success;
    }

    return true;
}

virtio_device_funs_t functions = {
    .device_reset = virtio_console_reset,
    .get_device_features = virtio_console_get_device_features,
    .set_driver_features = virtio_console_set_driver_features,
    .get_device_config = virtio_console_get_device_config,
    .set_device_config = virtio_console_set_device_config,
    .queue_notify = virtio_console_handle_tx,
};

static struct virtio_device *virtio_console_init(struct virtio_console_device *console, virtio_transport_type_t type,
                                                 size_t virq, serial_queue_handle_t *rxq, serial_queue_handle_t *txq,
                                                 int tx_ch)
{
    struct virtio_device *dev = &console->virtio_device;
    dev->regs.DeviceID = VIRTIO_DEVICE_ID_CONSOLE;
    dev->regs.VendorID = VIRTIO_DEV_VENDOR_ID;
    dev->transport_type = type;
    dev->funs = &functions;
    dev->vqs = console->vqs;
    dev->num_vqs = VIRTIO_CONSOLE_NUM_VIRTQ;
    dev->virq = virq;
    dev->device_data = console;

    console->rxq = rxq;
    console->txq = txq;
    console->tx_ch = tx_ch;

    return dev;
}

bool virtio_mmio_console_init(struct virtio_console_device *console, uintptr_t region_base, uintptr_t region_size,
                              size_t virq, serial_queue_handle_t *rxq, serial_queue_handle_t *txq, int tx_ch)
{
    struct virtio_device *dev = virtio_console_init(console, VIRTIO_TRANSPORT_MMIO, virq, rxq, txq, tx_ch);

    return virtio_mmio_register_device(dev, region_base, region_size, virq);
}

bool virtio_pci_console_init(struct virtio_console_device *console, uint32_t dev_slot, size_t virq,
                             serial_queue_handle_t *rxq, serial_queue_handle_t *txq, int tx_ch)
{
    struct virtio_device *dev = virtio_console_init(console, VIRTIO_TRANSPORT_PCI, virq, rxq, txq, tx_ch);

    dev->transport.pci.device_id = VIRTIO_PCI_CONSOLE_DEV_ID;
    dev->transport.pci.vendor_id = VIRTIO_PCI_VENDOR_ID;
    dev->transport.pci.device_class = PCI_CLASS_COMMUNICATION_OTHER;

    bool success = virtio_pci_alloc_dev_cfg_space(dev, dev_slot);
    assert(success);

    virtio_pci_alloc_memory_bar(dev, 0, VIRTIO_PCI_DEFAULT_BAR_SIZE);

    return virtio_pci_register_device(dev, virq);
}
