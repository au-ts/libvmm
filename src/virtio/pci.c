/*
 * Copyright 2025, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/pci.h>
#include <libvmm/virq.h>
#include <libvmm/virtio/virtio.h>
#include <libvmm/virtio/config.h>
#include <libvmm/virtio/virtq.h>
#include <libvmm/arch/aarch64/fault.h>

#define LOG_VIRTIO_PCI_INFO(...) do{ printf("%s|VIRTIO(PCI) INFO: ", microkit_name); printf(__VA_ARGS__); }while(0)
#define LOG_VIRTIO_PCI_ERR(...) do{ printf("%s|VIRTIO(PCI) ERROR: ", microkit_name); printf(__VA_ARGS__); }while(0)

// @billn pull in bug fixes from windows branch https://github.com/au-ts/libvmm/compare/main...windows

/* This is the default for virtio PCI devices as it allows plenty of space
 * for all the capabilities */
#define VIRTIO_PCI_DEFAULT_BAR_SIZE 0x4000

/* Since we place all the virtIO PCI registers into one BAR, we need to
 * differentiate them. */
#define VIRTIO_PCI_COMMON_CFG_BAR_OFF 0x0
#define VIRTIO_PCI_ISR_BAR_OFF        0x1000
#define VIRTIO_PCI_DEVICE_CFG_BAR_OFF 0x2000
#define VIRTIO_PCI_NOTIFY_CFG_BAR_OFF 0x3000

static bool handle_virtio_pci_set_status_flag(virtio_device_t *dev, uint32_t reg)
{
    bool success = true;

    // we only care about the new status
    dev->regs.Status &= reg;
    reg ^= dev->regs.Status;

    switch (reg) {
    case VIRTIO_CONFIG_S_RESET:
        dev->regs.Status = 0;
        dev->funs->device_reset(dev);
        break;

    case VIRTIO_CONFIG_S_ACKNOWLEDGE:
        // are we following the initialization protocol?
        if (dev->regs.Status == 0) {
            dev->regs.Status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
            // nothing to do from our side (as the virtio device).
        }
        break;

    case VIRTIO_CONFIG_S_DRIVER:
        // are we following the initialization protocol?
        if (dev->regs.Status & VIRTIO_CONFIG_S_ACKNOWLEDGE) {
            dev->regs.Status |= VIRTIO_CONFIG_S_DRIVER;
            // nothing to do from our side (as the virtio device).
        }
        break;

    case VIRTIO_CONFIG_S_FEATURES_OK:
        // are we following the initialization protocol?
        if (dev->regs.Status & VIRTIO_CONFIG_S_DRIVER) {
            // are features OK?
            dev->regs.Status |= (dev->features_happy ? VIRTIO_CONFIG_S_FEATURES_OK : 0);
        }
        break;

    case VIRTIO_CONFIG_S_DRIVER_OK:
        dev->regs.Status |= VIRTIO_CONFIG_S_DRIVER_OK;
        // probably do some san checks here
        break;

    case VIRTIO_CONFIG_S_FAILED:
        LOG_VIRTIO_PCI_ERR("received FAILED status from driver, giving up this device.\n");
        break;

    default:
        LOG_VIRTIO_PCI_ERR("unknown device status 0x%x.\n", reg);
        success = false;
    }
    return success;
}

static bool virtio_pci_common_reg_read(virtio_device_t *dev, size_t offset, uint32_t *data)
{
    bool success = true;

    switch (offset) {
    case VIRTIO_PCI_COMMON_DEV_FEATURE_SEL:
        *data = dev->regs.DeviceFeaturesSel;
        break;
    case VIRTIO_PCI_COMMON_DEV_FEATURE:
        success = dev->funs->get_device_features(dev, data);
        break;
    case VIRTIO_PCI_COMMON_NUM_QUEUES:
        *data = dev->num_vqs << 16; // @billn why << 16?
        break;
    case VIRTIO_PCI_COMMON_DEV_STATUS:
        *data = dev->regs.Status;
        break;
    case VIRTIO_PCI_COMMON_CFG_GENERATION:
        *data = dev->regs.ConfigGeneration;
        break;
    case VIRTIO_PCI_COMMON_Q_SIZE:
        *data = VIRTIO_DEFAULT_QUEUE_SIZE;
        break;
    case VIRTIO_PCI_COMMON_Q_ENABLE:
        *data = dev->vqs[dev->regs.QueueSel].ready;
        break;
    case VIRTIO_PCI_COMMON_Q_NOTIF_OFF:
        // proper way?
        *data = 1 << 16;
        break;
    default:
        LOG_VIRTIO_PCI_ERR("read operation is invalid or not implemented at offset 0x%lx of common_cfg\n", offset);
        success = false;
    }

    return success;
}

static bool virtio_pci_common_reg_write(virtio_device_t *dev, size_t offset, uint32_t data)
{
    bool success = true;

    switch (offset) {
    case VIRTIO_PCI_COMMON_DEV_FEATURE_SEL:
        dev->regs.DeviceFeaturesSel = data;
        break;
    case VIRTIO_PCI_COMMON_DRI_FEATURE_SEL:
        dev->regs.DriverFeaturesSel = data;
        break;
    case VIRTIO_PCI_COMMON_DRI_FEATURE:
        success = dev->funs->set_driver_features(dev, data);
        break;
    case VIRTIO_PCI_COMMON_DEV_STATUS:
        success = handle_virtio_pci_set_status_flag(dev, data);
        break;
    case VIRTIO_PCI_COMMON_Q_SELECT:
        dev->regs.QueueSel = data;
        break;
    case VIRTIO_PCI_COMMON_Q_SIZE:
        if (dev->regs.QueueSel < dev->num_vqs) {
            struct virtq *virtq = get_current_virtq_by_handler(dev);
            virtq->num = (unsigned int)data;
        } else {
            LOG_VIRTIO_PCI_ERR("invalid virtq index 0x%x (number of virtqs is 0x%lx) "
                               "given when accessing VIRTIO_PCI_COMMON_Q_SIZE\n",
                               dev->regs.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case VIRTIO_PCI_COMMON_Q_ENABLE:
        if (data == 0x1) {
            dev->vqs[dev->regs.QueueSel].ready = true;
            // the virtq is already in ram so we don't need to do any initiation
        }
        break;
    case VIRTIO_PCI_COMMON_Q_DESC_LO:
        if (dev->regs.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->regs.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->desc_gpa;
            ptr |= data;
            virtq->desc_gpa = (struct virtq_desc *)ptr;
        } else {
            LOG_VIRTIO_PCI_ERR("invalid virtq index 0x%x (number of virtqs is 0x%lx) "
                               "given when accessing VIRTIO_PCI_COMMON_Q_DESC_LO\n",
                               dev->regs.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case VIRTIO_PCI_COMMON_Q_DESC_HI:
        if (dev->regs.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->regs.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->desc_gpa;
            ptr |= (uintptr_t)data << 32;
            virtq->desc_gpa = (struct virtq_desc *)ptr;
        } else {
            LOG_VIRTIO_PCI_ERR("invalid virtq index 0x%x (number of virtqs is 0x%lx) "
                               "given when accessing VIRTIO_PCI_COMMON_Q_DESC_HI\n",
                               dev->regs.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case VIRTIO_PCI_COMMON_Q_AVAIL_LO:
        if (dev->regs.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->regs.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->avail_gpa;
            ptr |= data;
            virtq->avail_gpa = (struct virtq_avail *)ptr;
        } else {
            LOG_VIRTIO_PCI_ERR("invalid virtq index 0x%x (number of virtqs is 0x%lx) "
                               "given when accessing VIRTIO_PCI_COMMON_Q_AVAIL_LO\n",
                               dev->regs.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case VIRTIO_PCI_COMMON_Q_AVAIL_HI:
        if (dev->regs.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->regs.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->avail_gpa;
            ptr |= (uintptr_t)data << 32;
            virtq->avail_gpa = (struct virtq_avail *)ptr;
        } else {
            LOG_VIRTIO_PCI_ERR("invalid virtq index 0x%x (number of virtqs is 0x%lx) "
                               "given when accessing VIRTIO_PCI_COMMON_Q_AVAIL_HI\n",
                               dev->regs.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case VIRTIO_PCI_COMMON_Q_USED_LO:
        if (dev->regs.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->regs.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->used_gpa;
            ptr |= data;
            virtq->used_gpa = (struct virtq_used *)ptr;
        } else {
            LOG_VIRTIO_PCI_ERR("invalid virtq index 0x%x (number of virtqs is 0x%lx) "
                               "given when accessing VIRTIO_PCI_COMMON_Q_USED_LO\n",
                               dev->regs.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case VIRTIO_PCI_COMMON_Q_USED_HI:
        if (dev->regs.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->regs.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->used_gpa;
            ptr |= (uintptr_t)data << 32;
            virtq->used_gpa = (struct virtq_used *)ptr;
        } else {
            LOG_VIRTIO_PCI_ERR("invalid virtq index 0x%x (number of virtqs is 0x%lx) "
                               "given when accessing VIRTIO_PCI_COMMON_Q_USED_HI\n",
                               dev->regs.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    default:
        LOG_VIRTIO_PCI_ERR("write operation is invalid or not implemented at offset 0x%lx of common_cfg\n", offset);
        success = false;
    }

    return success;
}

static bool virtio_pci_isr_read(virtio_device_t *dev, size_t offset, uint32_t *data)
{
    *data = dev->regs.InterruptStatus;
    dev->regs.InterruptStatus = 0;
    return true;
}

static bool virtio_pci_device_reg_read(virtio_device_t *dev, size_t offset, uint32_t *data)
{
    return dev->funs->get_device_config(dev, offset, data);
}

static bool virtio_pci_device_reg_write(virtio_device_t *dev, size_t offset, uint32_t data)
{
    return dev->funs->set_device_config(dev, offset, data);
}

static bool virtio_pci_notify_reg_write(virtio_device_t *dev, size_t offset, uint32_t data)
{
    dev->regs.QueueNotify = data;
    dev->regs.QueueSel = offset / VIRTIO_PCI_NOTIF_OFF_MULTIPLIER;
    return dev->funs->queue_notify(dev);
}

bool virtio_pci_bar_fault_handle(pci_dev_handle_t pci_dev_handle, uint64_t bar_offset, bool is_read,
                                 int access_width_bytes, uint64_t *data, void *cookie)
{
    virtio_device_t *dev = (virtio_device_t *)cookie;
    assert(dev);

    bool success = false;

    switch (bar_offset) {
    case REG_RANGE(VIRTIO_PCI_COMMON_CFG_BAR_OFF, VIRTIO_PCI_ISR_BAR_OFF):
        if (is_read) {
            success = virtio_pci_common_reg_read(dev, bar_offset - VIRTIO_PCI_COMMON_CFG_BAR_OFF, (uint32_t *)data);
        } else {
            success = virtio_pci_common_reg_write(dev, bar_offset - VIRTIO_PCI_COMMON_CFG_BAR_OFF, (uint32_t)*data);
        }
        break;
    case REG_RANGE(VIRTIO_PCI_ISR_BAR_OFF, VIRTIO_PCI_DEVICE_CFG_BAR_OFF):
        if (is_read) {
            success = virtio_pci_isr_read(dev, bar_offset - VIRTIO_PCI_ISR_BAR_OFF, (uint32_t *)data);
        } else {
            LOG_VIRTIO_PCI_ERR("driver must not write to ISR\n");
            success = false;
        }
        break;
    case REG_RANGE(VIRTIO_PCI_DEVICE_CFG_BAR_OFF, VIRTIO_PCI_NOTIFY_CFG_BAR_OFF):
        if (is_read) {
            success = virtio_pci_device_reg_read(dev, bar_offset - VIRTIO_PCI_DEVICE_CFG_BAR_OFF, (uint32_t *)data);
        } else {
            success = virtio_pci_device_reg_write(dev, bar_offset - VIRTIO_PCI_DEVICE_CFG_BAR_OFF, (uint32_t)*data);
        }
        break;
    case REG_RANGE(VIRTIO_PCI_NOTIFY_CFG_BAR_OFF, VIRTIO_PCI_DEFAULT_BAR_SIZE):
        if (is_read) {
            LOG_VIRTIO_PCI_ERR("driver must not read from notify\n");
            success = false;
        } else {
            success = virtio_pci_notify_reg_write(dev, bar_offset - VIRTIO_PCI_NOTIFY_CFG_BAR_OFF, (uint32_t)*data);
        }
        break;
    default:
        /* Unreachable. */
        assert(0);
    }

    return success;
}

bool virtio_pci_register_device(virtio_device_t *dev, uint16_t pci_bus, uint16_t pci_dev, int virq)
{
    assert(dev->transport_type == VIRTIO_TRANSPORT_PCI);

    pci_device_register_data_t device_data = (pci_device_register_data_t) {
        .vendor_id = dev->transport.pci.vendor_id,
        .device_id = dev->transport.pci.device_id,
        .command = (PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER),
        .status = PCI_STATUS_CAP_LIST,
        .revision_id = VIRTIO_PCI_REVISION,
        .subclass = PCI_SUB_CLASS(dev->transport.pci.device_class),
        .class_code = PCI_CLASS_CODE(dev->transport.pci.device_class),
        .subsystem_vendor_id = dev->regs.VendorID,
        .subsystem_device_id = dev->regs.DeviceID,
    };

    pci_dev_handle_t handle = pci_register_device(pci_bus, pci_dev, 0, &device_data);
    if (handle == INVALID_PCI_DEVICE_HANDLE) {
        return false;
    }

    /* When the guest ACKs the IRQ there is nothing to do */
    if (!pci_register_device_irq(handle, virq, NULL, NULL)) {
        return false;
    }

    if (!pci_register_device_mmio_bar(handle, 0, VIRTIO_PCI_DEFAULT_BAR_SIZE, virtio_pci_bar_fault_handle,
                                      (void *)dev)) {
        return false;
    }

    /* Create all the virtIO specific capabilities */
    struct virtio_pci_cap common_cfg_cap = (struct virtio_pci_cap) {
        .cap_len = sizeof(struct virtio_pci_cap) + 2, // @billn sort out, include header legnth
        .cfg_type = VIRTIO_PCI_CAP_COMMON_CFG,
        .bar = 0,
        .id = 0,
        .offset = VIRTIO_PCI_COMMON_CFG_BAR_OFF,
        .length = 0x1000,
    };
    if (!pci_register_device_capability(handle, PCI_CAP_ID_VNDR, &common_cfg_cap, sizeof(struct virtio_pci_cap))) {
        return false;
    }

    struct virtio_pci_cap isr_cap = (struct virtio_pci_cap) {
        .cap_len = sizeof(struct virtio_pci_cap) + 2,
        .cfg_type = VIRTIO_PCI_CAP_ISR_CFG,
        .bar = 0,
        .id = 0,
        .offset = VIRTIO_PCI_ISR_BAR_OFF,
        .length = 0x1000,
    };
    if (!pci_register_device_capability(handle, PCI_CAP_ID_VNDR, &isr_cap, sizeof(struct virtio_pci_cap))) {
        return false;
    }

    struct virtio_pci_cap device_cfg_cap = (struct virtio_pci_cap) {
        .cap_len = sizeof(struct virtio_pci_cap) + 2,
        .cfg_type = VIRTIO_PCI_CAP_DEVICE_CFG,
        .bar = 0,
        .id = 0,
        .offset = VIRTIO_PCI_DEVICE_CFG_BAR_OFF,
        .length = 0x1000,
    };
    if (!pci_register_device_capability(handle, PCI_CAP_ID_VNDR, &device_cfg_cap, sizeof(struct virtio_pci_cap))) {
        return false;
    }

    struct virtio_pci_notify_cap ntfn_cap = (struct virtio_pci_notify_cap) {
        .cap =
            (struct virtio_pci_cap) {
                .cap_len = sizeof(struct virtio_pci_notify_cap) + 2,
                .cfg_type = VIRTIO_PCI_CAP_NOTIFY_CFG,
                .bar = 0,
                .id = 0,
                .offset = VIRTIO_PCI_NOTIFY_CFG_BAR_OFF,
                .length = 0x1000,
            },
        .notify_off_multiplier = VIRTIO_PCI_NOTIF_OFF_MULTIPLIER,
    };
    if (!pci_register_device_capability(handle, PCI_CAP_ID_VNDR, &ntfn_cap, sizeof(struct virtio_pci_notify_cap))) {
        return false;
    }

    return true;
}
