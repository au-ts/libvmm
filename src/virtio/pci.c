/*
 * Copyright 2025, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/guest.h>
#include <libvmm/virq.h>
#include <libvmm/virtio/virtio.h>
#include <libvmm/virtio/config.h>
#include <libvmm/virtio/virtq.h>
#include <libvmm/arch/aarch64/fault.h>

static struct pci_memory_resource registered_pci_memory_resource;
static struct pci_memory_bar global_memory_bars[MAX_MEM_BARS];

/**
 * Allocate a memory region from a memory bar for a capability.
 */
static uintptr_t pci_allocate_bar_memory(virtio_device_t *dev, uint8_t dev_bar_id, uint32_t size)
{
    // TODO: handle 64-bit memory bars??? assumes all bars are 32-bits for now

    // indice of the bar in global_memory_bars
    uint32_t idx = dev->transport.pci.mem_bar_ids[dev_bar_id];

    assert(global_memory_bars[idx].free_offset + size <= global_memory_bars[idx].size);

    uintptr_t allocated_offset = global_memory_bars[idx].free_offset;
    global_memory_bars[idx].free_offset = global_memory_bars[idx].free_offset + size;

    return allocated_offset;
}

/**
 * @config_space        the target device function to add capabilities to
 * @offset              offest to place the new cap at
 * @cfg_type            configuration type to be added
 */
static bool pci_add_virtio_capability(virtio_device_t *dev, uint8_t offset, uint8_t cfg_type,
                           uint8_t bar, uint8_t next_ptr)
{
    uintptr_t new_cap_addr = (uintptr_t)dev->transport.pci.ecam + offset;
    uint8_t len;            // length of the new cap
    uint32_t size = 0;      // size of the structure on the BAR, in bytes

    // Add the capability to the config space
    switch (cfg_type) {
    case VIRTIO_PCI_CAP_COMMON_CFG:
    case VIRTIO_PCI_CAP_ISR_CFG:
    case VIRTIO_PCI_CAP_DEVICE_CFG:
        len = sizeof(struct virtio_pci_cap);
        assert(offset + len < FUNC_CONFIG_SPACE_SIZE);
        size = 0x1000;
        break;
    case VIRTIO_PCI_CAP_NOTIFY_CFG:
        len = sizeof(struct virtio_pci_notify_cap);
        assert(offset + len < FUNC_CONFIG_SPACE_SIZE);
        // TODO: double-check this size
        size = 0x1000;
        struct virtio_pci_notify_cap *notify = (struct virtio_pci_notify_cap *)new_cap_addr;
        notify->notify_off_multiplier = VIRTIO_PCI_NOTIF_OFF_MULTIPLIER;
        break;
    case VIRTIO_PCI_CAP_PCI_CFG:
        len = sizeof(struct virtio_pci_cfg_cap);
        break;
    /* case VIRTIO_PCI_CAP_SHARED_MEMORY_CFG: */
    /* case VIRTIO_PCI_CAP_VENDOR_CFG: */
    default:
        LOG_VMM_ERR("Unimplemented capability type: 0x%x\n", cfg_type);
        return false;
    }

    uint32_t bar_offset = pci_allocate_bar_memory(dev, bar, size);

    struct virtio_pci_cap *new_cap = (struct virtio_pci_cap *)new_cap_addr;
    new_cap->cap_id = PCI_CAP_ID_VNDR;
    new_cap->next_ptr = next_ptr;
    new_cap->cap_len = len;
    new_cap->cfg_type = cfg_type;
    new_cap->bar = bar;
    new_cap->offset = bar_offset;
    new_cap->length = size;

    return true;
}

/**
 * @config_space        the target device function to add capabilities to
 * @cap_id              capability id to add
 * @cfg_type            configuration type to be added
 * @bar                 where the structure should be located
 */
static bool pci_add_capability(virtio_device_t *dev, uint8_t cap_id, uint8_t cfg_type, uint8_t bar)
{
    // Make sure the byte "cap_len" is within the region
    struct pci_config_space *config_space = dev->transport.pci.ecam;
    assert(config_space->cap_ptr + 2 < FUNC_CONFIG_SPACE_SIZE);

    uint8_t *last_cap = (uint8_t *)config_space + config_space->cap_ptr;
    uint8_t offset = config_space->cap_ptr + last_cap[2];
    bool success = true;

    switch (cap_id) {
    case PCI_CAP_ID_VNDR:
        success = pci_add_virtio_capability(dev, offset, cfg_type, bar, config_space->cap_ptr);
        break;
    default:
        success = false;
        LOG_VMM_ERR("Unimplementeed capability ID: 0x%x\n", cap_id);
    }

    if (!success) {
        LOG_VMM_ERR("Failed to add capability: ID (0x%x), type (0x%x), bar (0x%x)\n", cap_id, cfg_type, bar);
        return false;
    }

    // Update cap_ptr in config space
    config_space->cap_ptr = offset;
    return true;
}

void pci_add_memory_bar(virtio_device_t *dev, uint8_t bar_id, uint32_t size)
{
    assert(dev->transport_type == VIRTIO_TRANSPORT_PCI);
    // Assume there are only memory bars, and bar size needs to be 16-byte aligned
    assert((size & 0xFFFFFFF0) == size);

    uint32_t idx = 0;
    while (idx < MAX_MEM_BARS && global_memory_bars[idx].dev) idx++;

    global_memory_bars[idx].size = size;
    global_memory_bars[idx].free_offset = 0;
    global_memory_bars[idx].dev = dev;
    global_memory_bars[idx].idx = bar_id;

    struct pci_bar_memory_bits *new_mem_bar = (struct pci_bar_memory_bits *)&dev->transport.pci.ecam->bar[bar_id];
    new_mem_bar->base_address = 0;
}

/**
 * Look for the capability of a device by the offset on the memory resource
 * */
static struct virtio_pci_cap *find_pci_cap_by_offset(virtio_device_t *dev, uint8_t bar, uint32_t offset)
{
    struct virtio_pci_cap *cap = (struct virtio_pci_cap *)((uintptr_t)dev->transport.pci.ecam + dev->transport.pci.ecam->cap_ptr);
    while (cap != dev->transport.pci.ecam) {
        if (cap->cap_id == PCI_CAP_ID_VNDR && cap->bar == bar && (cap->offset <= offset && offset < cap->offset + cap->length)) {
            return cap;
        }
        cap = (struct virtio_pci_cap *)((uintptr_t)dev->transport.pci.ecam + cap->next_ptr);
    }
    return NULL;
}

static int handle_virtio_pci_set_status_flag(virtio_device_t *dev, uint32_t reg)
{
    int success = 1;

    // we only care about the new status
    dev->data.Status &= reg;
    reg ^= dev->data.Status;

    switch (reg) {
    case VIRTIO_CONFIG_S_RESET:
        dev->data.Status = 0;
        dev->funs->device_reset(dev);
        break;

    case VIRTIO_CONFIG_S_ACKNOWLEDGE:
        // are we following the initialization protocol?
        if (dev->data.Status == 0) {
            dev->data.Status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
            // nothing to do from our side (as the virtio device).
        }
        break;

    case VIRTIO_CONFIG_S_DRIVER:
        // are we following the initialization protocol?
        if (dev->data.Status & VIRTIO_CONFIG_S_ACKNOWLEDGE) {
            dev->data.Status |= VIRTIO_CONFIG_S_DRIVER;
            // nothing to do from our side (as the virtio device).
        }
        break;

    case VIRTIO_CONFIG_S_FEATURES_OK:
        // are we following the initialization protocol?
        if (dev->data.Status & VIRTIO_CONFIG_S_DRIVER) {
            // are features OK?
            dev->data.Status |= (dev->data.features_happy ? VIRTIO_CONFIG_S_FEATURES_OK : 0);
        }
        break;

    case VIRTIO_CONFIG_S_DRIVER_OK:
        dev->data.Status |= VIRTIO_CONFIG_S_DRIVER_OK;
        // probably do some san checks here
        break;

    case VIRTIO_CONFIG_S_FAILED:
        printf("VIRTIO PCI|INFO: received FAILED status from driver, giving up this device.\n");
        break;

    default:
        printf("VIRTIO PCI|INFO: unknown device status 0x%x.\n", reg);
        success = 0;
    }
    return success;
}

static bool virtio_pci_common_reg_read(virtio_device_t *dev, size_t vcpu_id, size_t offset, size_t fsr,
                                         seL4_UserContext *regs)
{
    uint32_t reg = 0;
    bool success = true;

    switch (offset) {
    case REG_RANGE(VIRTIO_PCI_COMMON_DEV_FEATURE_SEL, VIRTIO_PCI_COMMON_DEV_FEATURE):
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_DEV_FEATURE, VIRTIO_PCI_COMMON_DRI_FEATURE_SEL):
        success = dev->funs->get_device_features(dev, &reg);
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_DRI_FEATURE_SEL, VIRTIO_PCI_COMMON_DRI_FEATURE):
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_DRI_FEATURE, VIRTIO_PCI_COMMON_MSIX):
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_MSIX, VIRTIO_PCI_COMMON_NUM_QUEUES):
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_NUM_QUEUES, VIRTIO_PCI_COMMON_DEV_STATUS):
        // TODO: proper way?
        reg = dev->num_vqs << 16;
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_DEV_STATUS, VIRTIO_PCI_COMMON_CFG_GENERATION):
        reg = dev->data.Status;
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_CFG_GENERATION, VIRTIO_PCI_COMMON_Q_SIZE):
        reg = dev->data.ConfigGeneration;
        /* dev->data.ConfigGeneration += 1; */
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_SIZE, VIRTIO_PCI_COMMON_Q_ENABLE):
        reg = VIRTIO_PCI_QUEUE_SIZE;
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_ENABLE, VIRTIO_PCI_COMMON_Q_NOTIF_OFF):
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_NOTIF_OFF, VIRTIO_PCI_COMMON_Q_DESC_LO):
        // TODO: proper way?
        reg = 1 << 16;
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_DESC_LO, VIRTIO_PCI_COMMON_Q_DESC_HI):
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_DESC_HI, VIRTIO_PCI_COMMON_Q_AVAIL_LO):
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_AVAIL_LO, VIRTIO_PCI_COMMON_Q_AVAIL_HI):
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_AVAIL_HI, VIRTIO_PCI_COMMON_Q_USED_LO):
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_USED_LO, VIRTIO_PCI_COMMON_Q_USED_HI):
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_USED_HI, VIRTIO_PCI_COMMON_Q_NOTIF_DATA):
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_NOTIF_DATA, VIRTIO_PCI_COMMON_ADM_Q_IDX):
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_ADM_Q_IDX, VIRTIO_PCI_COMMON_END):
        break;
    }

    uint32_t mask = fault_get_data_mask(offset, fsr);
    // @ivanv: make it clearer that just passing the offset is okay,
    // possibly just fix the API
    fault_emulate_write(regs, offset, fsr, reg & mask);

    return success;
}

static bool virtio_pci_common_reg_write(virtio_device_t *dev, size_t vcpu_id, size_t offset, size_t fsr,
                                         seL4_UserContext *regs)
{
    bool success = true;
    uint32_t data = fault_get_data(regs, fsr);
    uint32_t mask = fault_get_data_mask(offset, fsr);
    /* Mask the data to write */
    /* TODO: Why? Given queue_sel is not the case */
    /* data &= mask; */

    switch (offset) {
    case REG_RANGE(VIRTIO_PCI_COMMON_DEV_FEATURE_SEL, VIRTIO_PCI_COMMON_DEV_FEATURE):
        dev->data.DeviceFeaturesSel = data;
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_DEV_FEATURE, VIRTIO_PCI_COMMON_DRI_FEATURE_SEL):
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_DRI_FEATURE_SEL, VIRTIO_PCI_COMMON_DRI_FEATURE):
        dev->data.DriverFeaturesSel = data;
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_DRI_FEATURE, VIRTIO_PCI_COMMON_MSIX):
        success = dev->funs->set_driver_features(dev, data);
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_MSIX, VIRTIO_PCI_COMMON_DEV_STATUS):
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_DEV_STATUS, VIRTIO_PCI_COMMON_CFG_GENERATION):
        success = handle_virtio_pci_set_status_flag(dev, data);
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_SELECT, VIRTIO_PCI_COMMON_Q_SIZE):
        dev->data.QueueSel = data;
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_SIZE, VIRTIO_PCI_COMMON_Q_ENABLE):
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_ENABLE, VIRTIO_PCI_COMMON_Q_NOTIF_OFF):
        if (data == 0x1) {
            dev->vqs[dev->data.QueueSel].ready = true;
            // the virtq is already in ram so we don't need to do any initiation
        }
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_DESC_LO, VIRTIO_PCI_COMMON_Q_DESC_HI):
        if (dev->data.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->data.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->desc;
            ptr |= data;
            virtq->desc = (struct virtq_desc *)ptr;
        } else {
            LOG_VMM_ERR("invalid virtq index 0x%lx (number of virtqs is 0x%lx) "
                        "given when accessing REG_VIRTIO_PCI_COMMAND_Q_DESC_LO\n", dev->data.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_DESC_HI, VIRTIO_PCI_COMMON_Q_AVAIL_LO):
        if (dev->data.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->data.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->desc;
            ptr |= (uintptr_t)data << 32;
            virtq->desc = (struct virtq_desc *)ptr;
        } else {
            LOG_VMM_ERR("invalid virtq index 0x%lx (number of virtqs is 0x%lx) "
                        "given when accessing REG_VIRTIO_MMIO_QUEUE_DESC_HIGH\n", dev->data.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_AVAIL_LO, VIRTIO_PCI_COMMON_Q_AVAIL_HI):
        if (dev->data.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->data.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->avail;
            ptr |= data;
            virtq->avail = (struct virtq_desc *)ptr;
        } else {
            LOG_VMM_ERR("invalid virtq index 0x%lx (number of virtqs is 0x%lx) "
                        "given when accessing REG_VIRTIO_PCI_COMMAND_Q_DESC_LO\n", dev->data.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_AVAIL_HI, VIRTIO_PCI_COMMON_Q_USED_LO):
        if (dev->data.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->data.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->avail;
            ptr |= (uintptr_t)data << 32;
            virtq->avail = (struct virtq_desc *)ptr;
        } else {
            LOG_VMM_ERR("invalid virtq index 0x%lx (number of virtqs is 0x%lx) "
                        "given when accessing REG_VIRTIO_PCI_COMMAND_Q_DESC_LO\n", dev->data.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_USED_LO, VIRTIO_PCI_COMMON_Q_USED_HI):
        if (dev->data.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->data.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->used;
            ptr |= data;
            virtq->used = (struct virtq_desc *)ptr;
        } else {
            LOG_VMM_ERR("invalid virtq index 0x%lx (number of virtqs is 0x%lx) "
                        "given when accessing REG_VIRTIO_PCI_COMMAND_Q_DESC_LO\n", dev->data.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_USED_HI, VIRTIO_PCI_COMMON_Q_NOTIF_DATA):
        if (dev->data.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->data.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->used;
            ptr |= (uintptr_t)data << 32;
            virtq->used = (struct virtq_desc *)ptr;
        } else {
            LOG_VMM_ERR("invalid virtq index 0x%lx (number of virtqs is 0x%lx) "
                        "given when accessing REG_VIRTIO_PCI_COMMAND_Q_DESC_LO\n", dev->data.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_NOTIF_DATA, VIRTIO_PCI_COMMON_ADM_Q_IDX):
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_ADM_Q_IDX, VIRTIO_PCI_COMMON_END):
        break;
    default:
        printf("VIRTIO PCI|ERR: driver should never write to the offset 0x%x\n", offset);
    }

    return success;
}

static bool virtio_pci_device_reg_read(virtio_device_t *dev, size_t vcpu_id, size_t offset, size_t fsr,
                                         seL4_UserContext *regs)
{
    uint32_t reg = 0;
    bool success = true;
    success = dev->funs->get_device_config(dev, offset, &reg);

    uint32_t mask = fault_get_data_mask(offset, fsr);
    // @ivanv: make it clearer that just passing the offset is okay,
    // possibly just fix the API
    fault_emulate_write(regs, offset, fsr, reg & mask);
    return success;
}

static bool virtio_pci_device_reg_write(virtio_device_t *dev, size_t vcpu_id, size_t offset, size_t fsr,
                                         seL4_UserContext *regs)
{
    bool success = true;
    uint32_t data = fault_get_data(regs, fsr);
    uint32_t mask = fault_get_data_mask(offset, fsr);
    /* Mask the data to write */
    data &= mask;
    success = dev->funs->set_device_config(dev, offset, data);
    return false;
}

static bool virtio_pci_notify_reg_read(virtio_device_t *dev, size_t vcpu_id, size_t offset, size_t fsr,
                                         seL4_UserContext *regs)
{
    printf("VIRTIO PCI|ERR: notfiy_cfg should not be read\n");
    return false;
}

static bool virtio_pci_notify_reg_write(virtio_device_t *dev, size_t vcpu_id, size_t offset, size_t fsr,
                                         seL4_UserContext *regs)
{
    bool success = true;
    uint32_t data = fault_get_data(regs, fsr);
    dev->data.QueueNotify = (uint32_t)data;

    /* TODO: virtio-pci does not write to queue_sel */
    dev->data.QueueSel = offset / VIRTIO_PCI_NOTIF_OFF_MULTIPLIER;
    success = dev->funs->queue_notify(dev);
    return success;
}

static bool virtio_pci_isr_reg_read(virtio_device_t *dev, size_t vcpu_id, size_t offset, size_t fsr,
                                         seL4_UserContext *regs)
{
    uint32_t reg = dev->data.InterruptStatus;
    uint32_t mask = fault_get_data_mask(offset, fsr);
    // @ivanv: make it clearer that just passing the offset is okay,
    // possibly just fix the API
    fault_emulate_write(regs, offset, fsr, reg & mask);
    dev->data.InterruptStatus = 0;
    return true;
}

static bool virtio_pci_isr_reg_write(virtio_device_t *dev, size_t vcpu_id, size_t offset, size_t fsr,
                                         seL4_UserContext *regs)
{
    printf("VIRTIO PCI|ERR: isr should not be written by the driver\n");
    return false;
}

static bool virtio_pci_bar_fault_handle(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data)
{
    uintptr_t vmm_addr = offset + registered_pci_memory_resource.vmm_addr;

    // Search for dev from global_memory_bars
    uint32_t i = 0;
    for (i = 0; i < MAX_MEM_BARS; i++) {
        if (global_memory_bars[i].vaddr <= vmm_addr &&
            vmm_addr < global_memory_bars[i].vaddr + global_memory_bars[i].size) {
            break;
        }
    }
    if (i == MAX_MEM_BARS) {
        LOG_VMM_ERR("Fault address 0x%x is not located within any registered memory bars\n", registered_pci_memory_resource.vm_addr + offset);
        return false;
    }

    // Search for the capability
    uint32_t bar_offset = vmm_addr - global_memory_bars[i].vaddr;
    virtio_device_t *dev = global_memory_bars[i].dev;
    struct  virtio_pci_cap *cap = find_pci_cap_by_offset(dev, global_memory_bars[i].idx, bar_offset);

    virtio_pci_cfg_exception_handler_t cfg_handler = NULL;
    switch (cap->cfg_type) {
        case VIRTIO_PCI_CAP_COMMON_CFG:
            cfg_handler = fault_is_read(fsr) ? virtio_pci_common_reg_read : virtio_pci_common_reg_write;
            break;
        case VIRTIO_PCI_CAP_DEVICE_CFG:
            cfg_handler = fault_is_read(fsr) ? virtio_pci_device_reg_read : virtio_pci_device_reg_write;
            break;
        case VIRTIO_PCI_CAP_NOTIFY_CFG:
            cfg_handler = fault_is_read(fsr) ? virtio_pci_notify_reg_read : virtio_pci_notify_reg_write;
            break;
        case VIRTIO_PCI_CAP_ISR_CFG:
            cfg_handler = fault_is_read(fsr) ? virtio_pci_isr_reg_read : virtio_pci_isr_reg_write;
            break;
        default:
            printf("Unimplemented bar fault handler for type: 0x%x\n", cap->cfg_type);
            break;
    }

    if (cfg_handler) {
        offset = offset - cap->offset;
        return cfg_handler(dev, vcpu_id, offset, fsr, regs);
    }
    return false;
}

void pci_add_memory_resource(uintptr_t vm_addr, uintptr_t vmm_addr, uint32_t size)
{
    // We assume that there is only one memory resource for each VM.
    registered_pci_memory_resource.vm_addr = vm_addr;
    registered_pci_memory_resource.vmm_addr = vmm_addr;
    registered_pci_memory_resource.size = size;

    bool success = fault_register_vm_exception_handler(vm_addr,
                                                  size,
                                                  &virtio_pci_bar_fault_handle,
                                                  NULL);


    if (!success) {
        LOG_VMM_ERR("Could not register virtual memory fault handler for pci device");
    }
}

static bool handle_virtio_ecam_reg_write(virtio_device_t *dev, size_t vcpu_id, size_t offset, size_t fsr,
                                         seL4_UserContext *regs)
{
    uint32_t data = fault_get_data(regs, fsr);

    switch (offset) {
        case REG_RANGE(PCI_CFG_OFFSET_COMMAND, PCI_CFG_OFFSET_STATUS): {
            struct pci_config_space *config_space = dev->transport.pci.ecam;
            config_space->command = data & (PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
            break;
        }
        case REG_RANGE(PCI_CFG_OFFSET_STATUS, PCI_CFG_OFFSET_BAR1): {
            struct pci_config_space *config_space = dev->transport.pci.ecam;
            config_space->status = data;
            break;
        }
        case REG_RANGE(PCI_CFG_OFFSET_BAR1, PCI_CFG_OFFSET_BAR2): {
            uint8_t dev_bar_id = (offset - PCI_CFG_OFFSET_BAR1) % 0x4;
            uint32_t global_bar_id = dev->transport.pci.mem_bar_ids[dev_bar_id];

            // Memory negotiation process:
            //     1. The driver writes all 1s to the BAR register.
            //     2. The device writes the size mask ~(size - 1) to the BAR register.
            //     3. The driver writes the original value (all 0s in our code) back
            //         to the BAR register. (no action from the device)
            //     4. The driver allocates memory from the memory resources, and writes
            //         the allocated address to the BAR register.
            //     5. The device parse the memory address and bookkeep it.
            if (global_memory_bars[global_bar_id].size) {
                if (data == 0xFFFFFFFF) {
                    struct pci_bar_memory_bits *bar = (struct pci_bar_memory_bits *)&(dev->transport.pci.ecam->bar[dev_bar_id]);
                    bar->base_address = (~(global_memory_bars[global_bar_id].size - 1)) >> 4;
                } else if (data != 0x0) {
                    struct pci_bar_memory_bits *bar = (struct pci_bar_memory_bits *)&dev->transport.pci.ecam->bar[dev_bar_id];
                    uintptr_t allocated_addr = data & 0xFFFFFFF0;   // Ignore control bits
                    bar->base_address = allocated_addr >> 4;        // 16-byte aligned
                    global_memory_bars[global_bar_id].vaddr = allocated_addr;
                }
            }
            break;

        }
    }
    return true;
}

static bool virtio_ecam_fault_handle(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data)
{
    virtio_device_t *dev = (virtio_device_t *) data;
    assert(dev);
    if (fault_is_read(fsr)) {
        uint32_t data = 0;
        if (offset < 0x1000) {
            uint32_t *reg = dev->transport.pci.ecam;
            data = reg[offset / 4];
        }

        uint32_t mask = fault_get_data_mask(offset, fsr);
        // @ivanv: make it clearer that just passing the offset is okay,
        // possibly just fix the API
        fault_emulate_write(regs, offset, fsr, data & mask);
        /* printf("[ecam read] offset: 0x%x, data: 0x%x, mask: 0x%x, fsr: 0x%x\n", offset, data & mask, mask, fsr); */
        return true;
    } else {
        return handle_virtio_ecam_reg_write(dev, vcpu_id, offset, fsr, regs);
    }
}

/*
 * If the guest acknowledges the virtual IRQ associated with the virtIO
 * device, there is nothing that we need to do.
 */
static void virtio_virq_default_ack(size_t vcpu_id, int irq, void *cookie) {}

bool virtio_pci_register_device(virtio_device_t *dev, int virq)
{
    assert(dev->transport_type == VIRTIO_TRANSPORT_PCI);

    // TODO: search for available device/function slots
    struct pci_config_space *config_space = dev->transport.pci.ecam;
    bool success;

    config_space->vendor_id = dev->transport.pci.vendor_id;
    config_space->device_id = dev->transport.pci.device_id;
    config_space->command = (PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
    config_space->status = PCI_STATUS_CAP_LIST;
    config_space->revision_id = VIRTIO_PCI_REVISION;
    // TODO: make the class variable
    config_space->subclass = PCI_SUB_CLASS(dev->transport.pci.device_class);
    config_space->class_code = PCI_CLASS_CODE(dev->transport.pci.device_class);
    config_space->subsystem_vendor_id = dev->data.VendorID;
    config_space->subsystem_device_id = dev->data.DeviceID;
    config_space->interrupt_line = 44;
    config_space->interrupt_pin = 0x1;

    config_space->cap_ptr = 0x40;
    success = pci_add_capability(dev, PCI_CAP_ID_VNDR, VIRTIO_PCI_CAP_COMMON_CFG, 0);
    assert(success);
    success = pci_add_capability(dev, PCI_CAP_ID_VNDR, VIRTIO_PCI_CAP_ISR_CFG, 0);
    assert(success);
    success = pci_add_capability(dev, PCI_CAP_ID_VNDR, VIRTIO_PCI_CAP_DEVICE_CFG, 0);
    assert(success);
    success = pci_add_capability(dev, PCI_CAP_ID_VNDR, VIRTIO_PCI_CAP_NOTIFY_CFG, 0);
    assert(success);
    success = pci_add_capability(dev, PCI_CAP_ID_VNDR, VIRTIO_PCI_CAP_PCI_CFG, 0);
    assert(success);

    success = fault_register_vm_exception_handler(dev->transport.pci.ecam_vm,
                                                  dev->transport.pci.ecam_size,
                                                  &virtio_ecam_fault_handle,
                                                  dev);


    if (!success) {
        LOG_VMM_ERR("Could not register virtual memory fault handler for pci device");
        return false;
    }

    /* Register the virtual IRQ that will be used to communicate from the device
     * to the guest. This assumes that the interrupt controller is already setup. */
    // @ivanv: we should check that (on AArch64) the virq is an SPI.
    // TODO: register after the driver writing to interrupt line
    success = virq_register(GUEST_VCPU_ID, virq, &virtio_virq_default_ack, NULL);
    assert(success);

    return success;
}
