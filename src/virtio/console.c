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
        dev->vqs[i].virtq.avail = 0;
        dev->vqs[i].virtq.used = 0;
        dev->vqs[i].virtq.desc = 0;
    }
    memset(&dev->regs, 0, sizeof(virtio_device_regs_t));
    // TODO: we should not be doing this here, and instead be calling the init function again or something
    // like that.
    dev->regs.DeviceID = VIRTIO_DEVICE_ID_CONSOLE;
    dev->regs.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
}

static bool virtio_console_get_device_features(struct virtio_device *dev, uint32_t *features)
{
    LOG_CONSOLE("operation: get device features\n");

    switch (dev->regs.DeviceFeaturesSel) {
    case 0:
        *features = BIT_LOW(VIRTIO_CONSOLE_F_MULTIPORT);
        break;
    case 1:
        *features = BIT_HIGH(VIRTIO_F_VERSION_1);
        break;
    default:
        *features = 0;
        LOG_CONSOLE_ERR("driver sets DeviceFeaturesSel to 0x%x, which doesn't make sense\n", dev->regs.DeviceFeaturesSel);
        return true;
    }

    return true;
}

static bool virtio_console_set_driver_features(struct virtio_device *dev, uint32_t features)
{
    LOG_CONSOLE("operation: set driver features\n");
    virtio_console_features_print(features);

    bool success = false;

    uint32_t device_features = BIT_LOW(VIRTIO_CONSOLE_F_MULTIPORT);

    switch (dev->regs.DriverFeaturesSel) {
    // feature bits 0 to 31
    case 0:
        /* We do not offer any features in the first 32-bit bits */
        success = (device_features & features) == features;
        break;
    // features bits 32 to 63
    case 1:
        success = (features == BIT_HIGH(VIRTIO_F_VERSION_1));
        break;
    default:
        LOG_CONSOLE_ERR("driver sets DriverFeaturesSel to 0x%x, which doesn't make sense\n", dev->regs.DriverFeaturesSel);
        success = true;
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

    struct virtio_console_device *console = device_state(dev);
    uintptr_t config_base_addr = (uintptr_t)&console->config;
    if (offset < sizeof(struct virtio_console_config) - 4) {
        memcpy((char *)config, (char *)(config_base_addr + offset), 4);
    }

    return true;
}

static bool virtio_console_set_device_config(struct virtio_device *dev, uint32_t offset, uint32_t config)
{
    LOG_CONSOLE("operation: set device config\n");
    return false;
}

static void virtio_console_handle_ctrl_rx(struct virtio_device *dev, struct virtio_console_control ctrl)
{
    struct virtio_queue_handler *vq = &dev->vqs[CTL_RX_QUEUE];
    assert(vq->ready);

    if (vq->last_idx != vq->virtq.avail->idx) {
        uint16_t desc_head = vq->virtq.avail->ring[vq->last_idx % vq->virtq.num];
        struct virtq_desc desc = vq->virtq.desc[desc_head];
        LOG_CONSOLE("processing control rx (0x%lx) with buffer [0x%lx..0x%lx)\n", desc_head, desc.addr, desc.addr + desc.len);

        assert(desc.len >= sizeof(struct virtio_console_control));

        struct virtio_console_control *dest_ctrl = (struct virtio_console_control *)gpa_to_vaddr(desc.addr);
        memcpy((void *)dest_ctrl, (void *)&ctrl, sizeof(struct virtio_console_control));

        struct virtq_used_elem used_elem = {desc_head, sizeof(struct virtio_console_control)};
        vq->virtq.used->ring[vq->virtq.used->idx % vq->virtq.num] = used_elem;
        vq->virtq.used->idx++;

        vq->last_idx++;
    } else {
        LOG_VMM_ERR("TODO\n");
        assert(false);
    }

    dev->regs.InterruptStatus = BIT_LOW(0);
    virq_inject(dev->virq);
}

static bool virtio_console_handle_ctrl_tx(struct virtio_device *dev) {
    LOG_CONSOLE("operation: handle control transmit\n");

    assert(dev->regs.QueueSel == CTL_TX_QUEUE);

    struct virtio_queue_handler *vq = &dev->vqs[CTL_TX_QUEUE];
    assert(vq->ready);

    while (vq->last_idx != vq->virtq.avail->idx) {
        uint16_t desc_idx = vq->virtq.avail->ring[vq->last_idx % vq->virtq.num];
        struct virtq_desc desc;
        /* Traverse chained descriptors */
        do {
            desc = vq->virtq.desc[desc_idx];
            LOG_CONSOLE("processing descriptor (0x%lx) with buffer [0x%lx..0x%lx)\n", desc_idx, desc.addr, desc.addr + desc.len);
            desc_idx = desc.next;
            assert(desc.len >= sizeof(struct virtio_console_control));

            struct virtio_console_control *ctrl = (struct virtio_console_control *)desc.addr;
            LOG_CONSOLE("ctrl message ID 0x%x, event: 0x%hx, value: 0x%hx\n", ctrl->id, ctrl->event, ctrl->value);

            if (ctrl->event == VIRTIO_CONSOLE_DEVICE_READY) {
                virtio_console_handle_ctrl_rx(dev, (struct virtio_console_control){
                    .id = 0,
                    .event = VIRTIO_CONSOLE_CON_PORT,
                    .value = 0,
                });
                virtio_console_handle_ctrl_rx(dev, (struct virtio_console_control){
                    .id = 0,
                    .event = VIRTIO_CONSOLE_PORT_ADD,
                    .value = 0,
                });
            }
        } while (desc.flags & VIRTQ_DESC_F_NEXT);

        struct virtq_used_elem used_elem = {vq->virtq.avail->ring[vq->last_idx % vq->virtq.num], 0};
        vq->virtq.used->ring[vq->virtq.used->idx % vq->virtq.num] = used_elem;
        vq->virtq.used->idx++;

        vq->last_idx++;
    }

    dev->regs.InterruptStatus = BIT_LOW(0);

    // bool success;
    virq_inject(dev->virq);
    // assert(success);

    // microkit_notify(console->tx_ch);

    return true;
}

static bool virtio_console_handle_tx(struct virtio_device *dev)
{
    LOG_CONSOLE("operation: handle transmit\n");

    if (dev->regs.QueueSel != TX_QUEUE) {
        LOG_VMM("QueueSel: 0x%x\n", dev->regs.QueueSel);
        return true;
    }

    assert(dev->num_vqs > TX_QUEUE);
    struct virtio_queue_handler *vq = &dev->vqs[TX_QUEUE];
    assert(vq->ready);
    struct virtio_console_device *console = device_state(dev);

    /* Transmit all available descriptors possible */
    LOG_CONSOLE("processing available buffers from index [0x%hx..0x%hx)\n", vq->last_idx, vq->virtq.avail->idx);
    bool transferred = false;
    while (vq->last_idx != vq->virtq.avail->idx && !serial_queue_full(console->txq, console->txq->queue->head)) {
        uint16_t desc_idx = vq->virtq.avail->ring[vq->last_idx % vq->virtq.num];
        struct virtq_desc desc;
        /* Traverse chained descriptors */
        do {
            desc = vq->virtq.desc[desc_idx];
            // @ivanv: to the debug logging, we should actually print out the buffer contents
            LOG_CONSOLE("processing descriptor (0x%lx) with buffer [0x%lx..0x%lx)\n", desc_idx, desc.addr, desc.addr + desc.len);

            uint32_t bytes_remain = desc.len;
            /* Copy all contiguous data */
            while (bytes_remain > 0 && !serial_queue_full(console->txq, console->txq->queue->head)) {
                uint32_t free = serial_queue_contiguous_free(console->txq);
                uint32_t to_transfer = MIN(bytes_remain, free);
                if (to_transfer) {
                    transferred = true;
                }

                if (free < bytes_remain) {
                    LOG_VMM_ERR("failed to copy all bytes at once (%u desc.len, %u free)\n", desc.len, free);
                }

                void *desc_vmm_addr = gpa_to_vaddr(desc.addr);
                memcpy(console->txq->data_region + (console->txq->queue->tail % console->txq->capacity),
                       (char *)(desc_vmm_addr + (desc.len - bytes_remain)), to_transfer);

                serial_update_shared_tail(console->txq, console->txq->queue->tail + to_transfer);
                bytes_remain -= to_transfer;
            }

            desc_idx = desc.next;

        } while (desc.flags & VIRTQ_DESC_F_NEXT && !serial_queue_full(console->txq, console->txq->queue->head));

        struct virtq_used_elem used_elem = {vq->virtq.avail->ring[vq->last_idx % vq->virtq.num], 0};
        vq->virtq.used->ring[vq->virtq.used->idx % vq->virtq.num] = used_elem;
        vq->virtq.used->idx++;

        vq->last_idx++;
    }

    /* While unlikely, it is possible that we could not consume any of the
     * available data. In this case we do not set the IRQ status. */
    if (transferred) {
        dev->regs.InterruptStatus = BIT_LOW(0);

        bool success;
#if defined(CONFIG_ARCH_AARCH64)
        success = virq_inject(dev->virq);
#elif defined(CONFIG_ARCH_X86_64)
        success = inject_ioapic_irq(0, dev->virq);
#endif
        // assert(success);

        microkit_notify(console->tx_ch);

        return true;
    }

    return true;
}

static bool virtio_console_queue_notify(struct virtio_device *dev)
{
    LOG_CONSOLE("operation: handle transmit\n");
    if (dev->regs.QueueSel == CTL_TX_QUEUE) {
        return virtio_console_handle_ctrl_tx(dev);
    } else if (dev->regs.QueueSel == TX_QUEUE) {
        return virtio_console_handle_tx(dev);
    } else {
        LOG_VMM_ERR("unknown queue notify\n");
        return false;
    }
}

bool virtio_console_handle_rx(struct virtio_console_device *console)
{
    LOG_CONSOLE("operation: handle rx\n");

    struct virtio_device *dev = &console->virtio_device;

    assert(dev->num_vqs > RX_QUEUE);

    /* Used to know whether to set the IRQ status. */
    bool transferred = false;

    struct virtio_queue_handler *vq = &dev->vqs[RX_QUEUE];
    if (!vq->ready) {
        /* It is common for the actual serial device to get RX before the guest
         * has initialised the console device, do nothing in this case.
         */
        return true;
    }

    LOG_CONSOLE("processing available buffers from index [0x%lx..0x%lx)\n", vq->last_idx, vq->virtq.avail->idx);
    while (vq->last_idx != vq->virtq.avail->idx && !serial_queue_empty(console->rxq, console->rxq->queue->head)) {
        transferred = true;

        uint16_t desc_head = vq->virtq.avail->ring[vq->last_idx % vq->virtq.num];
        struct virtq_desc desc = vq->virtq.desc[desc_head];
        LOG_CONSOLE("processing descriptor (0x%lx) with buffer [0x%lx..0x%lx)\n", desc_head, desc.addr, desc.addr + desc.len);
        uint32_t bytes_written = 0;
        char c;
        while (bytes_written < desc.len && !serial_dequeue(console->rxq, &c)) {
            *(char *)(gpa_to_vaddr(desc.addr) + bytes_written) = c;
            bytes_written++;
        }

        struct virtq_used_elem used_elem = {desc_head, bytes_written};
        vq->virtq.used->ring[vq->virtq.used->idx % vq->virtq.num] = used_elem;
        vq->virtq.used->idx++;

        vq->last_idx++;
    }

    /* While unlikely, it is possible that we could not consume any of the
     * available data. In this case we do not set the IRQ status. */
    if (transferred) {
        dev->regs.InterruptStatus = BIT_LOW(0);

        bool success;
#if defined(CONFIG_ARCH_AARCH64)
        success = virq_inject(dev->virq);
#elif defined(CONFIG_ARCH_X86_64)
        success = inject_ioapic_irq(0, dev->virq);
#endif
        // assert(success);

        return true;
    }

    return true;
}

virtio_device_funs_t functions = {
    .device_reset = virtio_console_reset,
    .get_device_features = virtio_console_get_device_features,
    .set_driver_features = virtio_console_set_driver_features,
    .get_device_config = virtio_console_get_device_config,
    .set_device_config = virtio_console_set_device_config,
    .queue_notify = virtio_console_queue_notify,
};

static struct virtio_device *virtio_console_init(struct virtio_console_device *console, size_t virq,
                                                 serial_queue_handle_t *rxq, serial_queue_handle_t *txq, int tx_ch)
{
    struct virtio_device *dev = &console->virtio_device;
    dev->regs.DeviceID = VIRTIO_DEVICE_ID_CONSOLE;
    dev->regs.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    dev->funs = &functions;
    dev->vqs = console->vqs;
    dev->num_vqs = VIRTIO_CONSOLE_NUM_VIRTQ;
    dev->virq = virq;
    dev->device_data = console;

    console->config.max_nr_ports = 1;

    console->rxq = rxq;
    console->txq = txq;
    console->tx_ch = tx_ch;

    return dev;
}

#if defined(CONFIG_ARCH_AARCH64)
bool virtio_mmio_console_init(struct virtio_console_device *console, uintptr_t region_base, uintptr_t region_size,
                              size_t virq, serial_queue_handle_t *rxq, serial_queue_handle_t *txq, int tx_ch)
{
    struct virtio_device *dev = virtio_console_init(console, virq, rxq, txq, tx_ch);

    dev->transport_type = VIRTIO_TRANSPORT_MMIO;
    return virtio_mmio_register_device(dev, region_base, region_size, virq);
}
#endif

bool virtio_pci_console_init(struct virtio_console_device *console, uint32_t dev_slot, size_t virq,
                             serial_queue_handle_t *rxq, serial_queue_handle_t *txq, int tx_ch)
{
    struct virtio_device *dev = virtio_console_init(console, virq, rxq, txq, tx_ch);

    dev->transport_type = VIRTIO_TRANSPORT_PCI;
    dev->transport.pci.device_id = VIRTIO_PCI_MODERN_BASE_DEVICE_ID + VIRTIO_DEVICE_ID_CONSOLE;
    dev->transport.pci.vendor_id = VIRTIO_PCI_VENDOR_ID;
    dev->transport.pci.device_class = PCI_CLASS_COMMUNICATION_OTHER;

    bool success = virtio_pci_alloc_dev_cfg_space(dev, dev_slot);
    assert(success);

    assert(virtio_pci_alloc_memory_bar(dev, 0, VIRTIO_PCI_DEFAULT_BAR_SIZE));

    assert(false);
    return false;
    // return pci_register_virtio_device(dev, virq);
}
