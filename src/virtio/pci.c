/*
 * Copyright 2025, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string.h>
#include <libvmm/guest.h>
#include <libvmm/virq.h>
#include <libvmm/virtio/virtio.h>
#include <libvmm/virtio/config.h>
#include <libvmm/virtio/virtq.h>

#if defined(CONFIG_ARCH_AARCH64)
#include <libvmm/arch/aarch64/fault.h>

#elif defined(CONFIG_ARCH_X86_64)
#include <libvmm/arch/x86_64/virq.h>
#include <libvmm/arch/x86_64/pci.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/util.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <libvmm/arch/x86_64/ioports.h>

extern struct pci_x86_passthrough pci_x86_passthrough_bookkeeping;

#endif

// @billn this should be moved to src/pci.c
// @billn I don't like how some functions are general PCI stuff but prefixed with virtio
// @billn I don't like how all of this is very virtio centric, need to make it generic to non
// virtio devices that we may have in the future such as passthrough GPUs or whatever.

static bool pci_config_space_read_access(uint8_t bus, uint8_t dev, uint8_t func, uint16_t reg_off, seL4_Word *data,
                                         int access_width_bytes);
static bool pci_config_space_write_access(uint8_t bus, uint8_t dev, uint8_t func, uint16_t reg_off, uint32_t data,
                                          int access_width_bytes);

#define LOG_PCI_INFO(...) do{ printf("%s|VIRTIO(PCI) INFO: ", microkit_name); printf(__VA_ARGS__); }while(0)
#define LOG_PCI_ERR(...) do{ printf("%s|VIRTIO(PCI) ERROR: ", microkit_name); printf(__VA_ARGS__); }while(0)

/* We assume that there is only one PCI node */
static struct virtio_pci_ecam global_pci_ecam;
static struct pci_memory_resource registered_pci_memory_resource;
static struct pci_memory_bar global_memory_bars[VIRTIO_PCI_MAX_MEM_BARS];

static virtio_device_t *virtio_pci_dev_table[VIRTIO_PCI_DEV_FUNC_MAX];

#if defined(CONFIG_ARCH_X86_64)
struct pci_x86_state {
    uint32_t selected_pio_addr_reg;
};

static struct pci_x86_state pci_x86_state;

// x86 PIO access
static bool pci_pio_addr_reg_enable(void)
{
    return !!(pci_x86_state.selected_pio_addr_reg >> 31);
}

static uint8_t pci_pio_addr_reg_bus(void)
{
    return (pci_x86_state.selected_pio_addr_reg >> 16) & 0x7f;
}

static uint8_t pci_pio_addr_reg_dev(void)
{
    return (pci_x86_state.selected_pio_addr_reg >> 11) & 0x1f;
}

static uint8_t pci_pio_addr_reg_func(void)
{
    return (pci_x86_state.selected_pio_addr_reg >> 8) & 0x7;
}

static uint8_t pci_pio_addr_reg_offset(void)
{
    return pci_x86_state.selected_pio_addr_reg & 0xff;
}

static void pci_invalid_pio_read(seL4_VCPUContext *vctx)
{
    uint64_t *vctx_raw = (uint64_t *)vctx;
    vctx_raw[RAX_IDX] = ~0ull;
}

// bool emulate_pci_config_space_access_pio(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read,
//                                          ioport_access_width_t access_width, struct pci_config_space *config_space)
// {
//     bool success = true;
//     uint64_t *vctx_raw = (uint64_t *)vctx;
//     uint8_t *config_bytes = (uint8_t *)config_space;
//     int port_offset = port_addr - PCI_CONFIG_DATA_START_PORT;

//     // caller must catch!
//     assert(pci_pio_addr_reg_enable());

//     int bytes_to_copy = ioports_access_width_to_bytes(access_width);
//     assert(bytes_to_copy);

//     if (!is_read) {
//         memcpy(&config_bytes[pci_pio_addr_reg_offset() + port_offset], &vctx_raw[RAX_IDX], bytes_to_copy);
//     } else {
//         memcpy(&vctx_raw[RAX_IDX], &config_bytes[pci_pio_addr_reg_offset() + port_offset], bytes_to_copy);
//     }

//     return success;
// }

// bool pci_x86_emulate_pio_access(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read,
//                                 ioport_access_width_t access_width)
// {
//     uint64_t *vctx_raw = (uint64_t *)vctx;

//     if (is_read) {
//         LOG_PCI_PIO("handling PCI config space mech #1 read at I/O 0x%lx\n", port_addr);
//     } else {
//         LOG_PCI_PIO("handling PCI config space mech #1 write at I/O 0x%lx, val 0x%lx\n", port_addr, vctx_raw[RAX_IDX]);
//     }

//     bool success = true;

//     if (port_addr >= PCI_CONFIG_ADDRESS_START_PORT && port_addr < PCI_CONFIG_ADDRESS_END_PORT) {
//         if (is_read) {
//             assert(port_addr == PCI_CONFIG_ADDRESS_START_PORT);
//             vctx_raw[RAX_IDX] = pci_x86_state.selected_pio_addr_reg;
//         } else {
//             uint32_t value = vctx_raw[RAX_IDX];
//             if (value >> 31) {
//                 // @billn revisit
//                 // vcpu_print_regs(0);
//                 // assert(port_addr == PCI_CONFIG_ADDRESS_START_PORT);
//                 pci_x86_state.selected_pio_addr_reg = value;

//                 LOG_PCI_PIO("selecting bus %d, device %d, func %d, reg_offset 0x%x\n",
//                             pci_host_bridge_pio_addr_reg_bus(), pci_host_bridge_pio_addr_reg_dev(),
//                             pci_host_bridge_pio_addr_reg_func(), pci_host_bridge_pio_addr_reg_offset());
//             }
//         }

//     } else if (port_addr >= PCI_CONFIG_DATA_START_PORT && port_addr < PCI_CONFIG_DATA_END_PORT) {
//         if (!pci_host_bridge_pio_addr_reg_enable()) {
//             pci_host_bridge_invalid_pio_read(vctx);
//         } else if (pci_host_bridge_pio_addr_reg_bus() > 0) {
//             // the real backing ECAM only have enough space for 1 bus
//             pci_host_bridge_invalid_pio_read(vctx);
//         } else {
//             // @Billn hack
//             uint8_t bus = pci_host_bridge_pio_addr_reg_bus();
//             uint8_t dev = pci_host_bridge_pio_addr_reg_dev();
//             uint8_t func = pci_host_bridge_pio_addr_reg_func();

//             uint32_t ecam_off = (bus << 20) | ((dev & 0x1f) << 15) | ((func & 0x7) << 12);
//             struct pci_config_space *config_space = (struct pci_config_space *)(global_pci_ecam.vmm_base + ecam_off);

//             LOG_VMM("global_pci_ecam.vmm_base = 0x%lx, ecam_off = 0x%lx\n", global_pci_ecam.vmm_base, ecam_off);

//             success = emulate_pci_config_space_access_pio(vctx, port_addr, is_read, access_width, config_space);
//         }
//     } else {
//         LOG_VMM_ERR("pci_x86_emulate_pio_access() called with unknown PIO addr 0x%x\n", port_addr);
//         success = false;
//     }

//     return success;
// }

static bool pci_pio_select_fault_handle(size_t vcpu_id, uint16_t port_offset, size_t qualification,
                                        seL4_VCPUContext *vctx, void *cookie)
{
    uint64_t is_read = qualification & BIT(3);
    uint16_t port_addr = (qualification >> 16) & 0xffff;

    if (is_read) {
        // LOG_VMM("PCI PIO reg select read -> 0x%x\n", pci_x86_state.selected_pio_addr_reg);

        assert(port_addr == PCI_CONFIG_ADDRESS_START_PORT);
        vctx->eax = pci_x86_state.selected_pio_addr_reg;
    } else {
        uint32_t value = vctx->eax;

        // LOG_VMM("PCI PIO reg select write <- 0x%x\n", value);

        if (value >> 31) {
            // @billn revisit
            // vcpu_print_regs(0);
            // assert(port_addr == PCI_CONFIG_ADDRESS_START_PORT);
            pci_x86_state.selected_pio_addr_reg = value;

            // LOG_PCI_PIO("selecting bus %d, device %d, func %d, reg_offset 0x%x\n",
            //             pci_host_bridge_pio_addr_reg_bus(), pci_host_bridge_pio_addr_reg_dev(),
            //             pci_host_bridge_pio_addr_reg_func(), pci_host_bridge_pio_addr_reg_offset());
        }
    }

    return true;
}

static bool pci_pio_data_fault_handle(size_t vcpu_id, uint16_t port_offset, size_t qualification,
                                      seL4_VCPUContext *vctx, void *cookie)
{
    // @billn make helpers
    uint64_t is_read = qualification & BIT(3);
    // uint16_t port_addr = (qualification >> 16) & 0xffff;
    ioport_access_width_t access_width = (ioport_access_width_t)(qualification & 0x7);

    uint64_t is_string = qualification & BIT(4);
    assert(!is_string);

    if (!pci_pio_addr_reg_enable()) {
        pci_invalid_pio_read(vctx);
        return true;
    } else if (pci_pio_addr_reg_bus() > 0) {
        // the real backing ECAM only have enough space for 1 bus
        pci_invalid_pio_read(vctx);
        return true;
    } else {
        // LOG_VMM("PCI PIO accessing bus %d, dev %d, func %d\n", bus, dev, func);

        uint8_t bus = pci_pio_addr_reg_bus();
        uint8_t dev = pci_pio_addr_reg_dev();
        uint8_t func = pci_pio_addr_reg_func();
        uint8_t reg_off = pci_pio_addr_reg_offset() + port_offset;

        if (is_read) {
            return pci_config_space_read_access(bus, dev, func, reg_off, &(vctx->eax),
                                                ioports_access_width_to_bytes(access_width));
        } else {
            return pci_config_space_write_access(bus, dev, func, reg_off, vctx->eax,
                                                 ioports_access_width_to_bytes(access_width));
        }
    }
}

#endif

// Translate PCI Geographic Address, represented as Bus:Device.Function, to an offset in the ECAM region
static uint32_t pci_geo_addr_to_ecam_offset(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (bus << 20) | ((dev & 0x1f) << 15) | ((func & 0x7) << 12);
}

// // Translate an offset to the ECAM region into a `struct pci_config_space *` regardless of what register
// // is being accessed
// static struct pci_config_space *pci_ecam_offset_to_config_space()

static struct pci_config_space *virtio_pci_find_dev_cfg_space(virtio_device_t *dev)
{
    uint32_t dev_table_idx = dev->transport.pci.dev_table_idx;
    assert(dev_table_idx < VIRTIO_PCI_DEV_FUNC_MAX);

    if (virtio_pci_dev_table[dev_table_idx] == NULL) {
        return NULL;
    }

    uint16_t bus_id = dev_table_idx / VIRTIO_PCI_DEVS_PER_BUS / VIRTIO_PCI_FUNCS_PER_DEV;
    uint8_t dev_slot = (dev_table_idx % VIRTIO_PCI_DEVS_PER_BUS) / VIRTIO_PCI_FUNCS_PER_DEV;
    uint8_t func_id = dev_table_idx % VIRTIO_PCI_FUNCS_PER_DEV;

    uint32_t offset = pci_geo_addr_to_ecam_offset(bus_id, dev_slot, func_id);
    if ((offset + (1 << 15)) > global_pci_ecam.size) {
        LOG_PCI_ERR("ECAM area for 0x%4x:0x%2x:0x%2x,0x%x is invalid \n", 0, bus_id, dev_slot, func_id);
    }

    return (struct pci_config_space *)(global_pci_ecam.vmm_base + offset);
}

/**
 * Allocate a memory region from a memory bar for a capability.
 */
static uintptr_t pci_allocate_bar_memory(virtio_device_t *dev, uint8_t dev_bar_id, uint32_t size)
{
    // indice of the bar in global_memory_bars
    uint32_t idx = dev->transport.pci.mem_bar_ids[dev_bar_id];

    assert(global_memory_bars[idx].free_offset + size <= global_memory_bars[idx].size);

    uintptr_t allocated_offset = global_memory_bars[idx].free_offset;
    global_memory_bars[idx].free_offset = global_memory_bars[idx].free_offset + size;

    return allocated_offset;
}

static bool pci_add_virtio_capability(virtio_device_t *dev, struct virtio_pci_cap *cap, uint8_t offset,
                                      uint8_t cfg_type, uint8_t bar)
{
    uint8_t len; // length of the new cap
    uint32_t size = 0; // size of the structure on the BAR, in bytes

    // Add the capability to the config space
    switch (cfg_type) {
    case VIRTIO_PCI_CAP_COMMON_CFG:
    case VIRTIO_PCI_CAP_ISR_CFG:
    case VIRTIO_PCI_CAP_DEVICE_CFG:
        len = sizeof(struct virtio_pci_cap);
        assert(offset + len < VIRTIO_PCI_FUNC_CFG_SPACE_SIZE);
        size = 0x1000;
        break;
    case VIRTIO_PCI_CAP_NOTIFY_CFG:
        len = sizeof(struct virtio_pci_notify_cap);
        assert(offset + len < VIRTIO_PCI_FUNC_CFG_SPACE_SIZE);
        size = 0x1000;
        struct virtio_pci_notify_cap *notify = (struct virtio_pci_notify_cap *)cap;
        // "For example, if notifier_off_multiplier is 0, the device uses the same Queue Notify address for all queues."
        notify->notify_off_multiplier = 0;
        break;
    case VIRTIO_PCI_CAP_PCI_CFG:
        len = sizeof(struct virtio_pci_cfg_cap);
        break;
    /* case VIRTIO_PCI_CAP_SHARED_MEMORY_CFG: */
    /* case VIRTIO_PCI_CAP_VENDOR_CFG: */
    default:
        LOG_PCI_ERR("Unimplemented capability type: 0x%x\n", cfg_type);
        return false;
    }

    uint32_t bar_offset = pci_allocate_bar_memory(dev, bar, size);

    cap->cap_id = PCI_CAP_ID_VNDR;
    cap->cap_len = len;
    cap->cfg_type = cfg_type;
    cap->bar = bar;
    cap->offset = bar_offset;
    cap->length = size;

    return true;
}

static void pci_dump_capabilities(struct pci_config_space *config_space)
{
    LOG_VMM("=========== Dumping all PCI capabilities\n");
    LOG_VMM("first cap offset 0x%x\n", config_space->cap_ptr);

    struct virtio_pci_cap *curr_cap = (struct virtio_pci_cap *)((uintptr_t)config_space + config_space->cap_ptr);
    while (curr_cap->cap_id == PCI_CAP_ID_VNDR) {
        LOG_VMM("dumping cap: 0x%p\n", curr_cap);
        LOG_VMM("type: %d\n", curr_cap->cfg_type);
        LOG_VMM("next: 0x%x\n", curr_cap->next_ptr);
        curr_cap = (struct virtio_pci_cap *)((uintptr_t)config_space + curr_cap->next_ptr);
    }
    LOG_VMM("=========== finished dumping all PCI capabilities\n");
}

static bool pci_add_capability(virtio_device_t *dev, uint8_t cap_id, uint8_t cfg_type, uint8_t bar)
{
    // Make sure the byte "cap_len" is within the region
    struct pci_config_space *config_space = virtio_pci_find_dev_cfg_space(dev);
    assert(config_space->cap_ptr + 2 < VIRTIO_PCI_FUNC_CFG_SPACE_SIZE);

    bool success = true;

    struct virtio_pci_cap *new_cap = NULL;
    uint8_t new_cap_offset = 0;
    if (config_space->cap_ptr == 0) {
        config_space->cap_ptr = 0x40;
        new_cap = (struct virtio_pci_cap *)((uintptr_t)config_space + config_space->cap_ptr);
        new_cap_offset = config_space->cap_ptr;
    } else {
        struct virtio_pci_cap *curr_cap = (struct virtio_pci_cap *)((uintptr_t)config_space + config_space->cap_ptr);
        assert(config_space->cap_ptr == 0x40);

        while (curr_cap->next_ptr) {
            curr_cap = (struct virtio_pci_cap *)((uintptr_t)config_space + curr_cap->next_ptr);
        }

        assert(curr_cap->next_ptr == 0);

        uint8_t new_cap_offset = ((uintptr_t)curr_cap - (uintptr_t)config_space) + curr_cap->cap_len;
        new_cap = (struct virtio_pci_cap *)((uintptr_t)config_space + new_cap_offset);
        curr_cap->next_ptr = new_cap_offset;
    }

    switch (cap_id) {
    case PCI_CAP_ID_VNDR:
        success = pci_add_virtio_capability(dev, new_cap, new_cap_offset, cfg_type, bar);
        break;
    default:
        success = false;
        LOG_PCI_ERR("Unimplementeed capability ID: 0x%x\n", cap_id);
    }

    if (!success) {
        LOG_PCI_ERR("Failed to add capability: ID (0x%x), type (0x%x), bar (0x%x)\n", cap_id, cfg_type, bar);
        return false;
    }

    return true;
}

bool virtio_pci_alloc_memory_bar(virtio_device_t *dev, uint8_t bar_id, uint32_t size)
{
    assert(dev->transport_type == VIRTIO_TRANSPORT_PCI);
    // Assume there are only memory bars, and bar size needs to be 16-byte aligned
    assert((size & 0xFFFFFFF0) == size);

    uint32_t idx = 0;
    while (idx < VIRTIO_PCI_MAX_MEM_BARS && global_memory_bars[idx].dev) idx++;
    if (idx == VIRTIO_PCI_MAX_MEM_BARS) {
        LOG_PCI_ERR("No more available memory bar slots. Please increase VIRTIO_PCI_MAX_MEM_BARS\n");
        return false;
    }

    if (registered_pci_memory_resource.free_offset + size > registered_pci_memory_resource.size) {
        LOG_PCI_ERR("Could not allocate 0x%x bytes for new memory bar. 0x%x bytes left.\n", size,
                    registered_pci_memory_resource.size - registered_pci_memory_resource.free_offset);
        return false;
    }

    global_memory_bars[idx].size = size;
    global_memory_bars[idx].free_offset = 0;
    global_memory_bars[idx].dev = dev;
    global_memory_bars[idx].idx = bar_id;

    dev->transport.pci.mem_bar_ids[bar_id] = idx;

    struct pci_config_space *config_space = virtio_pci_find_dev_cfg_space(dev);
    struct pci_bar_memory_bits *new_mem_bar = (struct pci_bar_memory_bits *)&config_space->bar[bar_id];
    new_mem_bar->base_address = 0;

    return true;
}

/**
 * Look for the capability of a device by the offset on the memory resource
 * */
static struct virtio_pci_cap *find_pci_cap_by_offset(virtio_device_t *dev, uint8_t bar, uint32_t offset)
{
    struct pci_config_space *config_space = virtio_pci_find_dev_cfg_space(dev);
    struct virtio_pci_cap *cap = (struct virtio_pci_cap *)((uintptr_t)config_space + config_space->cap_ptr);
    while (cap != (struct virtio_pci_cap *)config_space) {
        if (cap->cap_id == PCI_CAP_ID_VNDR && cap->bar == bar
            && (cap->offset <= offset && offset < cap->offset + cap->length)) {
            return cap;
        }
        cap = (struct virtio_pci_cap *)((uintptr_t)config_space + cap->next_ptr);
    }
    return NULL;
}

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
        LOG_PCI_INFO("received FAILED status from driver, giving up on this device (ID 0x%x).\n", dev->regs.DeviceID);
        break;

    default:
        LOG_PCI_INFO("unknown device status 0x%x.\n", reg);
        success = false;
    }
    return success;
}

static bool virtio_pci_common_reg_read(virtio_device_t *dev, size_t vcpu_id, size_t offset, uint32_t *data)
{
    bool success = true;

    switch (offset) {
    case VIRTIO_PCI_COMMON_DEV_FEATURE_SEL:
        *data = dev->regs.DeviceFeaturesSel;
        break;
    // TODO: handle on MMIO as well
    case VIRTIO_PCI_COMMON_DRI_FEATURE:
        *data = dev->regs.DriverFeatures;
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_DEV_FEATURE, VIRTIO_PCI_COMMON_DRI_FEATURE_SEL):
        success = dev->funs->get_device_features(dev, data);
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_NUM_QUEUES, VIRTIO_PCI_COMMON_DEV_STATUS):
        *data = dev->num_vqs;
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_DEV_STATUS, VIRTIO_PCI_COMMON_CFG_GENERATION):
        *data = dev->regs.Status;
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_CFG_GENERATION, VIRTIO_PCI_COMMON_Q_SIZE):
        *data = dev->regs.ConfigGeneration;
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_SIZE, VIRTIO_PCI_COMMON_Q_ENABLE):
        *data = VIRTIO_PCI_QUEUE_SIZE;
        dev->vqs[dev->regs.QueueSel].virtq.num = VIRTIO_PCI_QUEUE_SIZE;
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_ENABLE, VIRTIO_PCI_COMMON_Q_NOTIF_OFF):
        *data = dev->vqs[dev->regs.QueueSel].ready;
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_NOTIF_OFF, VIRTIO_PCI_COMMON_Q_DESC_LO):
        // proper way?
        *data = 0;
        break;
    default:
        LOG_PCI_ERR("read operation is invalid or not implemented at offset 0x%x of common_cfg\n", offset);
        // TODO: do not check in
        success = true;
    }

    return success;
}

static bool virtio_pci_common_reg_write(virtio_device_t *dev, size_t vcpu_id, size_t offset, uint32_t data)
{
    bool success = true;

    // LOG_VMM("accessing device 0x%x, offset 0x%x, data 0x%x, q select 0x%x\n", dev->regs.DeviceID, offset, data, dev->regs.QueueSel);

    switch (offset) {
    case VIRTIO_PCI_COMMON_DEV_FEATURE:
        LOG_PCI_ERR("ignoring write of 0x%x to PCI device_feature, read-only register\n", data);
        break;
    case VIRTIO_PCI_COMMON_NUM_QUEUES:
        LOG_PCI_ERR("ignoring write of 0x%x to PCI num_queues, read-only register\n", data);
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_DEV_FEATURE_SEL, VIRTIO_PCI_COMMON_DEV_FEATURE):
        dev->regs.DeviceFeaturesSel = data;
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_DRI_FEATURE_SEL, VIRTIO_PCI_COMMON_DRI_FEATURE):
        dev->regs.DriverFeaturesSel = data;
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_DRI_FEATURE, VIRTIO_PCI_COMMON_MSIX):
        success = dev->funs->set_driver_features(dev, data);
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_DEV_STATUS, VIRTIO_PCI_COMMON_CFG_GENERATION):
        success = handle_virtio_pci_set_status_flag(dev, data);
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_SELECT, VIRTIO_PCI_COMMON_Q_SIZE):
        dev->regs.QueueSel = data;
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_SIZE, VIRTIO_PCI_COMMON_Q_ENABLE):
        LOG_VMM("pci virtio vq size reconfigure 0x%x\n", data);
        assert(data == VIRTIO_PCI_QUEUE_SIZE);
        // what if different size configured?
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_ENABLE, VIRTIO_PCI_COMMON_Q_NOTIF_OFF):
        if (data == 0x1) {
            dev->vqs[dev->regs.QueueSel].ready = true;
            // the virtq is already in ram so we don't need to do any initiation

            // The guest writes GPA into these registers, we must translate it into VMM virtual address
            // @billn currently doesnt work on aarch64 as it lacks this util function
            dev->vqs[dev->regs.QueueSel].virtq.avail = gpa_to_vaddr((uint64_t)dev->vqs[dev->regs.QueueSel].virtq.avail);
            dev->vqs[dev->regs.QueueSel].virtq.used = gpa_to_vaddr((uint64_t)dev->vqs[dev->regs.QueueSel].virtq.used);
            dev->vqs[dev->regs.QueueSel].virtq.desc = gpa_to_vaddr((uint64_t)dev->vqs[dev->regs.QueueSel].virtq.desc);
        }
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_DESC_LO, VIRTIO_PCI_COMMON_Q_DESC_HI):
        if (dev->regs.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->regs.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->desc;
            ptr |= data;
            virtq->desc = (struct virtq_desc *)ptr;
        } else {
            LOG_PCI_ERR("invalid virtq index 0x%lx (number of virtqs is 0x%lx) "
                        "given when accessing VIRTIO_PCI_COMMON_Q_DESC_LO\n",
                        dev->regs.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_DESC_HI, VIRTIO_PCI_COMMON_Q_AVAIL_LO):
        if (dev->regs.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->regs.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->desc;
            ptr |= (uintptr_t)data << 32;
            virtq->desc = (struct virtq_desc *)ptr;
        } else {
            LOG_PCI_ERR("invalid virtq index 0x%lx (number of virtqs is 0x%lx) "
                        "given when accessing VIRTIO_PCI_COMMON_Q_DESC_HI\n",
                        dev->regs.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_AVAIL_LO, VIRTIO_PCI_COMMON_Q_AVAIL_HI):
        if (dev->regs.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->regs.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->avail;
            ptr |= data;
            virtq->avail = (struct virtq_avail *)ptr;
        } else {
            LOG_PCI_ERR("invalid virtq index 0x%lx (number of virtqs is 0x%lx) "
                        "given when accessing VIRTIO_PCI_COMMON_Q_AVAIL_LO\n",
                        dev->regs.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_AVAIL_HI, VIRTIO_PCI_COMMON_Q_USED_LO):
        if (dev->regs.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->regs.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->avail;
            ptr |= (uintptr_t)data << 32;
            virtq->avail = (struct virtq_avail *)ptr;
        } else {
            LOG_PCI_ERR("invalid virtq index 0x%lx (number of virtqs is 0x%lx) "
                        "given when accessing VIRTIO_PCI_COMMON_Q_AVAIL_HI\n",
                        dev->regs.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_USED_LO, VIRTIO_PCI_COMMON_Q_USED_HI):
        if (dev->regs.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->regs.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->used;
            ptr |= data;
            virtq->used = (struct virtq_used *)ptr;
        } else {
            LOG_PCI_ERR("invalid virtq index 0x%lx (number of virtqs is 0x%lx) "
                        "given when accessing VIRTIO_PCI_COMMON_Q_USED_LO\n",
                        dev->regs.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    case REG_RANGE(VIRTIO_PCI_COMMON_Q_USED_HI, VIRTIO_PCI_COMMON_Q_NOTIF_DATA):
        if (dev->regs.QueueSel < dev->num_vqs) {
            struct virtq *virtq = &dev->vqs[dev->regs.QueueSel].virtq;
            uintptr_t ptr = (uintptr_t)virtq->used;
            ptr |= (uintptr_t)data << 32;
            virtq->used = (struct virtq_used *)ptr;
        } else {
            LOG_PCI_ERR("invalid virtq index 0x%lx (number of virtqs is 0x%lx) "
                        "given when accessing VIRTIO_PCI_COMMON_Q_USED_HI\n",
                        dev->regs.QueueSel, dev->num_vqs);
            success = false;
        }
        break;
    default:
        LOG_PCI_ERR("write operation is invalid or not implemented at offset 0x%x of common_cfg\n", offset);
    }

    return success;
}

static bool virtio_pci_device_reg_read(virtio_device_t *dev, size_t vcpu_id, size_t offset, uint32_t *data)
{
    bool success = dev->funs->get_device_config(dev, offset, data);
    return success;
}

static bool virtio_pci_device_reg_write(virtio_device_t *dev, size_t vcpu_id, size_t offset, uint32_t data)
{
    bool success = dev->funs->set_device_config(dev, offset, data);
    return success;
}

static bool virtio_pci_notify_reg_read(virtio_device_t *dev, size_t vcpu_id, size_t offset, uint32_t *data)
{
    LOG_PCI_ERR("notfiy_cfg should not be read\n");
    return false;
}

static bool virtio_pci_notify_reg_write(virtio_device_t *dev, size_t vcpu_id, size_t offset, uint32_t data)
{
    dev->regs.QueueNotify = data;
    dev->regs.QueueSel = data;
    bool success = dev->funs->queue_notify(dev);
    return success;
}

static bool virtio_pci_isr_reg_read(virtio_device_t *dev, size_t vcpu_id, size_t offset, uint32_t *data)
{
    *data = dev->regs.InterruptStatus;
    return true;
}

static bool virtio_pci_isr_reg_write(virtio_device_t *dev, size_t vcpu_id, size_t offset, uint32_t data)
{
    LOG_PCI_ERR("isr should not be written by the driver\n");
    return false;
}

#if defined(CONFIG_ARCH_AARCH64)
static bool virtio_pci_bar_fault_handle(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *cookie)
#elif defined(CONFIG_ARCH_X86_64)
static bool virtio_pci_bar_fault_handle(size_t vcpu_id, size_t offset, size_t qualification,
                                        decoded_instruction_ret_t decoded_ins, seL4_VCPUContext *vctx, void *cookie)
#endif
{
    bool success = true;
    uintptr_t vmm_addr = offset + registered_pci_memory_resource.vmm_addr;

    // Search for dev from global_memory_bars
    uint32_t i = 0;
    for (i = 0; i < VIRTIO_PCI_MAX_MEM_BARS; i++) {
        if (global_memory_bars[i].vaddr <= vmm_addr
            && vmm_addr < global_memory_bars[i].vaddr + global_memory_bars[i].size) {
            break;
        }
    }
    if (i == VIRTIO_PCI_MAX_MEM_BARS) {
        LOG_PCI_ERR("Fault address 0x%x is not located within any registered memory bars\n",
                    registered_pci_memory_resource.vm_addr + offset);
        return false;
    }

    // Search for the capability
    uint32_t bar_offset = vmm_addr - global_memory_bars[i].vaddr;
    virtio_device_t *dev = global_memory_bars[i].dev;
    struct virtio_pci_cap *cap = find_pci_cap_by_offset(dev, global_memory_bars[i].idx, bar_offset);

    uint64_t data;
#if defined(CONFIG_ARCH_AARCH64)
    bool is_read = fault_is_read(fsr);

    if (!is_read) {
        data = fault_get_data(regs, fsr);
        // @billn revisit why doing this cause the VMM to crash because `virtq->avail` wasn't set
        // uint32_t mask = fault_get_data_mask(offset, fsr);
        // data &= mask;
    }
#elif defined(CONFIG_ARCH_X86_64)
    bool is_read = ept_fault_is_read(qualification);
    if (!is_read) {
        assert(mem_write_get_data(decoded_ins, qualification, vctx, &data));
    }
#endif

    switch (cap->cfg_type) {
    case VIRTIO_PCI_CAP_COMMON_CFG:
        if (is_read) {
            success = virtio_pci_common_reg_read(dev, vcpu_id, bar_offset - cap->offset, (uint32_t *)&data);
        } else {
            success = virtio_pci_common_reg_write(dev, vcpu_id, bar_offset - cap->offset, data);
        }
        break;
    case VIRTIO_PCI_CAP_DEVICE_CFG:
        if (is_read) {
            success = virtio_pci_device_reg_read(dev, vcpu_id, bar_offset - cap->offset, (uint32_t *)&data);
        } else {
            success = virtio_pci_device_reg_write(dev, vcpu_id, bar_offset - cap->offset, data);
        }
        break;
    case VIRTIO_PCI_CAP_NOTIFY_CFG:
        if (is_read) {
            success = virtio_pci_notify_reg_read(dev, vcpu_id, bar_offset - cap->offset, (uint32_t *)&data);
        } else {
            success = virtio_pci_notify_reg_write(dev, vcpu_id, bar_offset - cap->offset, data);
        }
        break;
    case VIRTIO_PCI_CAP_ISR_CFG:
        if (is_read) {
            success = virtio_pci_isr_reg_read(dev, vcpu_id, bar_offset - cap->offset, (uint32_t *)&data);
        } else {
            success = virtio_pci_isr_reg_write(dev, vcpu_id, bar_offset - cap->offset, data);
        }
        break;
    default:
        LOG_PCI_INFO("Unimplemented bar fault handler for type: 0x%x\n", cap->cfg_type);
        success = false;
        break;
    }

    if (success && is_read) {
#if defined(CONFIG_ARCH_AARCH64)
        uint32_t mask = fault_get_data_mask(offset, fsr);
        fault_emulate_write(regs, offset, fsr, data & mask);
#elif defined(CONFIG_ARCH_X86_64)
        assert(mem_read_set_data(decoded_ins, qualification, vctx, data));
#endif
    }

    return success;
}

bool virtio_pci_register_memory_resource(uintptr_t vm_addr, uintptr_t vmm_addr, uint32_t size)
{
    // We assume that there is only one memory resource for each VM.
    registered_pci_memory_resource.vm_addr = vm_addr;
    registered_pci_memory_resource.vmm_addr = vmm_addr;
    registered_pci_memory_resource.size = size;
    registered_pci_memory_resource.free_offset = 0;

    bool success;

#if defined(CONFIG_ARCH_AARCH64)
    success = fault_register_vm_exception_handler(vm_addr, size, &virtio_pci_bar_fault_handle, NULL);
    if (!success) {
        LOG_PCI_ERR("Could not register virtual memory fault handler for PCI BAR");
    }
#elif defined(CONFIG_ARCH_X86_64)
    success = fault_register_ept_exception_handler(vm_addr, size, &virtio_pci_bar_fault_handle, NULL);
    if (!success) {
        LOG_PCI_ERR("Could not register EPT fault handler for PCI BAR");
    }
#endif

    return success;
}

static bool pci_config_space_read_access(uint8_t bus, uint8_t dev, uint8_t func, uint16_t reg_off, seL4_Word *data,
                                         int access_width_bytes)
{
// @billn hack cdrom passthrough
#if defined(CONFIG_ARCH_X86_64)
    if (pci_x86_passthrough_bookkeeping.ata_passthrough && bus == 0 && dev == 1 && func == 1) {
        return pci_x86_config_space_read_from_native(access_width_bytes, 0, 1, 1, reg_off, (uint32_t *)data);
    }
#endif

    uint32_t config_space_ecam_off = pci_geo_addr_to_ecam_offset(bus, dev, func);
    uint8_t *bytes = (uint8_t *)(global_pci_ecam.vmm_base + config_space_ecam_off + reg_off);
    *data = 0;
    memcpy(data, bytes, access_width_bytes);
    return true;
}

void log_pci_bar(uint8_t bus, uint8_t dev, uint8_t func, uint32_t reg_off) {
    LOG_VMM("PCI BAR access (%d:%d.%d) at 0x%x\n", bus, dev, func, reg_off);
}

static bool pci_config_space_write_access(uint8_t bus, uint8_t dev, uint8_t func, uint16_t reg_off, uint32_t data,
                                          int access_width_bytes)
{
// @billn hack cdrom passthrough
#if defined(CONFIG_ARCH_X86_64)
    if (pci_x86_passthrough_bookkeeping.ata_passthrough && bus == 0 && dev == 1 && func == 1) {
        return pci_x86_config_space_write_to_native(access_width_bytes, 0, 1, 1, reg_off, data);
    }

    // @billn hack, prevent corruption of host and ISA bridge config space
    if (bus == 0 && dev == 0 && reg_off < 0x40) {
        return true;
    }
    if (bus == 0 && dev == 1 && reg_off < 0x40) {
        return true;
    }
#endif

    uint32_t config_space_ecam_off = pci_geo_addr_to_ecam_offset(bus, dev, func);
    struct pci_config_space *config_space = (struct pci_config_space *)(global_pci_ecam.vmm_base
                                                                        + config_space_ecam_off);

    // @billn should we allow unchecked writes to all registers??

    switch (reg_off) {
    case REG_RANGE(PCI_CFG_OFFSET_BAR1, PCI_CFG_OFFSET_CARDBUS): {

        uint32_t dev_table_idx = ((bus * VIRTIO_PCI_DEVS_PER_BUS) + dev & 0x1F) * VIRTIO_PCI_FUNCS_PER_DEV + (func & 7);
        virtio_device_t *dev_handle = virtio_pci_dev_table[dev_table_idx];

        // hack for the host bridge, it doesnt have a dev_handle
        if (!dev_handle) {
            LOG_VMM("PCI host bridge hack (offset: 0x%lx, data: 0x%lx)\n", reg_off, data);
            return true;
        }

        // LOG_VMM("%d:%d.%d BAR negotiation write 0x%lx\n", bus, dev, func, data);

        uint8_t dev_bar_id = (reg_off - PCI_CFG_OFFSET_BAR1) / 0x4;
        uint32_t global_bar_id = dev_handle->transport.pci.mem_bar_ids[dev_bar_id];

        log_pci_bar(bus, dev, func, reg_off);

        // Memory negotiation process:
        //     1. The driver writes all 1s to the BAR register.
        //     2. The device writes the size mask ~(size - 1) to the BAR register.
        //     3. The driver writes the original value (all 0s in our code) back
        //         to the BAR register. (no action from the device)
        //     4. The driver allocates memory from the memory resources, and writes
        //         the allocated address to the BAR register.
        //     5. The device parse the memory address and bookkeep it.

        struct pci_bar_memory_bits *bar = (struct pci_bar_memory_bits *)&config_space->bar[dev_bar_id];
        uint32_t size = global_memory_bars[global_bar_id].size;
        if (dev_bar_id != 0) {
            size = 0;
        }
        LOG_VMM("PCI BAR offset: 0x%lx, (before) bar->base_address: 0x%lx, data: 0x%lx, size: 0x%lx\n", reg_off, bar->base_address, data, size);
        if (dev_bar_id != 0) {
            if (data == 0xFFFFFFFF) {
                uint32_t inverse_size = (~(size - 1));
                LOG_VMM("PCI BAR inverse_size: 0x%x\n", inverse_size);
                bar->base_address = inverse_size >> 4;
            } else if (data != 0x0) {
                LOG_VMM("PCI unused bar writing non-zero 0x%x\n", data);
                uintptr_t allocated_addr = data & 0xFFFFFFF0; // Ignore control bits
                bar->base_address = allocated_addr >> 4; // 16-byte aligned
            } else {
                bar->base_address &= 0xf;
                LOG_VMM("writing zero to BAR offset 0x%lx\n", reg_off);
            }
        } else {
            if (data == 0xFFFFFFFF) {
                uint32_t inverse_size = (~(size - 1));
                LOG_VMM("PCI BAR inverse_size: 0x%x\n", inverse_size);
                bar->base_address = inverse_size >> 4;
            } else if (data != 0x0) {
                uintptr_t allocated_addr = data & 0xFFFFFFF0; // Ignore control bits
                bar->base_address = allocated_addr >> 4; // 16-byte aligned
                global_memory_bars[global_bar_id].vaddr = allocated_addr - registered_pci_memory_resource.vm_addr
                                                        + registered_pci_memory_resource.vmm_addr;
            } else {
                bar->base_address &= 0xf;
                LOG_VMM("writing zero to BAR offset 0x%lx\n", reg_off);
            }
        }
        LOG_VMM("PCI BAR offset: 0x%lx, (after) bar->base_address: 0x%lx, data: 0x%lx, size: 0x%lx\n", reg_off, bar->base_address, data, size);
        break;
    }
    case PCI_CFG_OFFSET_COMMAND: {
        // TODO: This is still incorrect as it assumes that the device does not implement
        // I/O space. This behaviour should instead be dependent on the kind of PCI device.
        data |= (PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
        uint8_t *bytes = (uint8_t *)((uintptr_t)config_space + reg_off);
        memcpy(bytes, &data, MIN(2, access_width_bytes));
        break;
    }
    case PCI_CFG_OFFSET_STATUS:
        data |= PCI_STATUS_CAP_LIST;
        uint8_t *bytes = (uint8_t *)((uintptr_t)config_space + reg_off);
        memcpy(bytes, &data, MIN(2, access_width_bytes));
        break;
    case PCI_CFG_OFFSET_LATENCY_TIMER:
    case PCI_CFG_OFFSET_CAP_PTR:
    case PCI_CFG_OFFSET_IRQ_LINE: {
        // TODO: don't do this and handle each case
        LOG_VMM("unchecked write into PCI config reg off 0x%lx, value: 0x%lx\n", reg_off, data);
        uint8_t *bytes = (uint8_t *)((uintptr_t)config_space + reg_off);
        memcpy(bytes, &data, access_width_bytes);
        break;
    }
    default:
        // TODO: redo this by checking the PCI specification for what should be a checked write, ignored
        // write, or passed-through.
        // Make sure to refer to the PCI sepcification sections.
        if (reg_off >= 0x40 && reg_off < 0x100) {
            uint8_t *bytes = (uint8_t *)((uintptr_t)config_space + reg_off);
            memcpy(bytes, &data, access_width_bytes);
        } else if (reg_off == 0x30) {
            // Ignore
            break;
        } else {
            // TODO: do not assert and handle this gracefully instead.
            LOG_VMM_ERR("tried to write to PCI config offset 0x%x, width: %d, value: 0x%x\n", reg_off, access_width_bytes, data);
        }
    }
    return true;
}

#if defined(CONFIG_ARCH_AARCH64)
static bool pci_ecam_handle_access(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data)
{
    // LOG_VMM("handling virtio exam fault offset 0x%x\n", offset);
    // uint32_t dev_table_idx = (((offset >> 20) * VIRTIO_PCI_DEVS_PER_BUS) + (offset >> 15) & 0x1F)
    //                            * VIRTIO_PCI_FUNCS_PER_DEV
    //                        + ((offset >> 12) & 7);
    // virtio_device_t *dev = virtio_pci_dev_table[dev_table_idx];

    uint8_t bus = (offset >> 20) & 0xff;
    uint8_t dev = (offset >> 15) & 0x1f;
    uint8_t func = (offset >> 12) & 0x7;
    uint16_t reg_off = offset & 0xfff;

    // LOG_VMM("handling virtio exam fault bus 0x%x dev 0x%x func 0x%x reg_off 0x%x\n", bus, dev, func, reg_off);

    // @billn @ivanv we don't understand why this worked for non 32-bit aligned access. E.g. register offset 0x6
    if (fault_is_read(fsr)) {
        uint32_t mask = fault_get_data_mask(offset, fsr);
        uint32_t data;
        bool success = pci_config_space_read_access(bus, dev, func, (reg_off / 4) * 4, &data, 4);
        if (success) {
            fault_emulate_write(regs, offset, fsr, data & mask);
        }
        if ((offset & 0xFF) % 4) {
            LOG_VMM("unaligned pci reg off 0x%x, read val masked 0x%x, read val 0x%x\n", (offset & 0xFF), data & mask,
                    data);
        }
        return success;
    } else {
        return pci_config_space_write_access(bus, dev, func, reg_off, fault_get_data(regs, fsr), 4);
    }

    // if (fault_is_read(fsr)) {
    //     uint32_t mask = fault_get_data_mask(offset, fsr);
    //     uint32_t data = ~0ull;
    //     if (dev) {
    //         uint32_t *reg = (uint32_t *)virtio_pci_find_dev_cfg_space(dev);
    //         data = reg[(offset & 0xFF) / 4];
    //     }
    //     fault_emulate_write(regs, offset, fsr, data & mask);
    //     return true;
    // } else {
    //     return handle_virtio_ecam_reg_write(dev, offset & 0xFF, fault_get_data(regs, fsr));
    // }
}
#elif defined(CONFIG_ARCH_X86_64)

// // @billn a hack for the host and ISA bridge
// static bool handle_ecam_access_unchecked(size_t vcpu_id, size_t offset, size_t qualification,
//                                          memory_instruction_data_t decoded_mem_ins, seL4_VCPUContext *vctx,
//                                          void *cookie)
// {
//     assert(((offset >> 20) & 0xff) == 0);
//     assert((((offset >> 15) & 0x1f) == 0) || (((offset >> 15) & 0x1f) == 1));

//     uint64_t *vctx_raw = (uint64_t *)vctx;
//     int access_width_bytes = mem_access_width_to_bytes(decoded_mem_ins.access_width);
//     uint8_t *bytes = (uint8_t *)(global_pci_ecam.vmm_base + offset);
//     if (ept_fault_is_read(qualification)) {
//         // LOG_PCI_ECAM("Reading via ECAM: bus %d, device %d, func %d, reg_offset 0x%x, width %d\n", bus, dev, func,
//         //              reg_off, access_width_bytes);
//         memcpy(&vctx_raw[decoded_mem_ins.target_reg], bytes, access_width_bytes);

//     } else {
//         // LOG_PCI_ECAM("Writing via ECAM: bus %d, device %d, func %d, reg_offset 0x%x, width %d = value 0x%lx\n", bus,
//         //              dev, func, reg_off, access_width_bytes, vctx_raw[decoded_mem_ins.target_reg]);
//         memcpy(bytes, &vctx_raw[decoded_mem_ins.target_reg], access_width_bytes);
//     }

//     return true;
// }

static bool pci_ecam_handle_access(size_t vcpu_id, size_t offset, size_t qualification,
                                   decoded_instruction_ret_t decoded_ins, seL4_VCPUContext *vctx, void *cookie)
{
    // uint8_t bus = offset >> 20;
    // uint8_t dev_id = (offset >> 15) & 0x1f;

    // // @billn hack
    // if (bus == 0 && (dev_id == 0 || dev_id == 1)) {
    //     return handle_ecam_access_unchecked(vcpu_id, offset, qualification, decoded_mem_ins, vctx, cookie);
    // }

    // uint32_t dev_table_idx = (((offset >> 20) * VIRTIO_PCI_DEVS_PER_BUS) + (offset >> 15) & 0x1F)
    //                            * VIRTIO_PCI_FUNCS_PER_DEV
    //                        + ((offset >> 12) & 7);
    // virtio_device_t *dev = virtio_pci_dev_table[dev_table_idx];
    int access_width_bytes = mem_access_width_to_bytes(decoded_ins);
    assert(access_width_bytes != 0 && access_width_bytes <= 4);

    uint8_t bus = (offset >> 20) & 0xff;
    uint8_t dev = (offset >> 15) & 0x1f;
    uint8_t func = (offset >> 12) & 0x7;
    uint16_t reg_off = offset & 0xfff;

    if (ept_fault_is_read(qualification)) {
        uint64_t value;
        bool success = pci_config_space_read_access(bus, dev, func, reg_off, &value, access_width_bytes);
        assert(mem_read_set_data(decoded_ins, qualification, vctx, value));
        return success;
    } else {
        uint64_t value;
        assert(mem_write_get_data(decoded_ins, qualification, vctx, &value));
        return pci_config_space_write_access(bus, dev, func, reg_off, (uint32_t)value, access_width_bytes);
    }
}

#endif

/*
 * If the guest acknowledges the virtual IRQ associated with the virtIO
 * device, there is nothing that we need to do.
 */
static void virtio_virq_default_ack(size_t vcpu_id, int irq, void *cookie)
{
}
static void virtio_virq_ioapic_default_ack(int ioapic, int pin, void *cookie)
{
}

bool virtio_pci_alloc_dev_cfg_space(virtio_device_t *dev, uint8_t dev_slot)
{
    // Always use the only bus and func for each device.
    uint16_t bus_id = 0;
    uint8_t func_id = 0;

    uint32_t dev_table_idx = ((bus_id * VIRTIO_PCI_DEVS_PER_BUS) + dev_slot) * VIRTIO_PCI_FUNCS_PER_DEV + func_id;

    if (virtio_pci_dev_table[dev_table_idx]) {
        LOG_PCI_ERR("The config space for 0x%4x:0x%2x:0x%2x,0x%x has been taken.\n", 0, bus_id, dev_slot, func_id);
        return false;
    }

    dev->transport.pci.dev_table_idx = dev_table_idx;
    virtio_pci_dev_table[dev_table_idx] = dev;
    return true;
}

bool virtio_pci_ecam_init(uintptr_t ecam_base_vm, uintptr_t ecam_base_vmm, uint32_t ecam_size)
{
    global_pci_ecam.vm_base = ecam_base_vm;
    global_pci_ecam.vmm_base = ecam_base_vmm;
    global_pci_ecam.size = ecam_size;

    memset((void *)ecam_base_vmm, 0, ecam_size);
    int bus = 0;
    for (int dev = 0; dev <= 0x1f; dev++) {
        for (int func = 0; func <= 0x7; func++) {
            struct pci_config_space *config_space =
                (struct pci_config_space *)(ecam_base_vmm + pci_geo_addr_to_ecam_offset(bus, dev, func));
            config_space->vendor_id = 0xffff;
        }
    }

    bool success = false;

#if defined(CONFIG_ARCH_AARCH64)
    success = fault_register_vm_exception_handler(ecam_base_vm, ecam_size, &pci_ecam_handle_access, NULL);
    if (!success) {
        LOG_PCI_ERR("Could not register virtual memory fault handler for PCI ECAM area!\n");
    }
#elif defined(CONFIG_ARCH_X86_64)
    success = fault_register_ept_exception_handler(ecam_base_vm, ecam_size, &pci_ecam_handle_access, NULL);
    if (!success) {
        LOG_PCI_ERR("Could not register EPT fault handler for PCI ECAM area!\n");
        return success;
    }

    success = fault_register_pio_exception_handler(PCI_CONFIG_ADDRESS_START_PORT, PCI_CONFIG_ADDRESS_PORT_SIZE,
                                                   pci_pio_select_fault_handle, NULL);
    if (!success) {
        LOG_PCI_ERR("Could not register PIO fault handler for PCI mech #1 select register!\n");
        return success;
    }
    success = fault_register_pio_exception_handler(PCI_CONFIG_DATA_START_PORT, PCI_CONFIG_DATA_PORT_SIZE,
                                                   pci_pio_data_fault_handle, NULL);
    if (!success) {
        LOG_PCI_ERR("Could not register PIO fault handler for PCI mech #1 data register!\n");
        return success;
    }
#endif

    return success;
}

bool pci_register_virtio_device(virtio_device_t *dev, int virq)
{
    assert(dev->transport_type == VIRTIO_TRANSPORT_PCI);

    struct pci_config_space *config_space = virtio_pci_find_dev_cfg_space(dev);

    config_space->vendor_id = dev->transport.pci.vendor_id;
    config_space->device_id = dev->transport.pci.device_id;
    config_space->command = (PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
    config_space->status = PCI_STATUS_CAP_LIST;
    config_space->revision_id = VIRTIO_PCI_REVISION;
    config_space->subclass = PCI_SUB_CLASS(dev->transport.pci.device_class);
    config_space->class_code = PCI_CLASS_CODE(dev->transport.pci.device_class);
    config_space->subsystem_vendor_id = dev->regs.VendorID;
    config_space->subsystem_device_id = dev->regs.DeviceID;

    // Always use dev's first interrupt pin specified in interrupt-map
    config_space->interrupt_pin = 0x1;
    config_space->interrupt_line = virq;

    bool success = true;
    config_space->cap_ptr = 0;
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

    /* Register the virtual IRQ that will be used to communicate from the device
     * to the guest. This assumes that the interrupt controller is already setup. */
#if defined(CONFIG_ARCH_AARCH64)
    success = virq_register(GUEST_BOOT_VCPU_ID, virq, &virtio_virq_default_ack, NULL);
#elif defined(CONFIG_ARCH_X86_64)
    success = virq_ioapic_register(0, virq, &virtio_virq_ioapic_default_ack, NULL);
#endif
    assert(success);

    return success;
}

bool pci_ecam_add_device(uint8_t bus, uint8_t dev, uint8_t func, struct pci_config_space *config_space)
{
    if (!global_pci_ecam.vmm_base) {
        LOG_VMM_ERR("PCI is not initialised\n");
    }

    uint32_t offset = pci_geo_addr_to_ecam_offset(bus, dev, func);
    struct pci_config_space *config_space_dest = (struct pci_config_space *)(global_pci_ecam.vmm_base + offset);
    memcpy(config_space_dest, config_space, sizeof(struct pci_config_space));
    return true;
}