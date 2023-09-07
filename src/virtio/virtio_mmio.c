/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <sel4cp.h>
#include "../util/util.h"
#include "../arch/aarch64/fault.h"
#include "include/config/virtio_config.h"
#include "virtio_mmio.h"
#include "virtio_mem.h"
#include "virtio_net_emul.h"
// #include "virtio_gpu_emul.h"

// @jade: add some giant comments about this file
// generic virtio mmio emul interface

#define REG_RANGE(r0, r1)   r0 ... (r1 - 1)

virtio_emul_handler_t *get_emul_handler_by_address(uint64_t addr)
{
    switch (addr)
    {
    case REG_RANGE(VIRTIO_NET_ADDRESS_START, VIRTIO_NET_ADDRESS_END):
        return get_virtio_net_emul_handler();
    // case REG_RANGE(VIRTIO_GPU_ADDRESS_START, VIRTIO_GPU_ADDRESS_END):
    //     return get_virtio_gpu_emul_handler();
    default:
        return NULL;
    }
}

static uint32_t get_device_offset(uint64_t addr)
{
    switch(addr) {
    case REG_RANGE(VIRTIO_NET_ADDRESS_START, VIRTIO_NET_ADDRESS_END):
        return VIRTIO_NET_ADDRESS_START;
    // case REG_RANGE(VIRTIO_GPU_ADDRESS_START, VIRTIO_GPU_ADDRESS_END):
    //     return VIRTIO_GPU_ADDRESS_START;
    default:
        return -1;
    }
}

struct vring *get_current_vring_by_handler(virtio_emul_handler_t *emul_handler)
{
    assert(emul_handler->data.QueueSel < VIRTIO_MMIO_NET_NUM_VIRTQUEUE);
    return &emul_handler->vqs[emul_handler->data.QueueSel].vring;
}

/*
 * Protocol for device status changing can be found in:
 * https://docs.oasis-open.org/virtio/virtio/v1.2/csd01/virtio-v1.2-csd01.html,
 * 3.1 Device Initialization
*/
int handle_virtio_mmio_get_status_flag(virtio_emul_handler_t *emul_handler, uint32_t *retreg)
{
    *retreg = emul_handler->data.Status;
    // printf("\"%s\"|VIRTIO MMIO|INFO: Status is 0x%x.\n", sel4cp_name, emul_handler->data.Status);
    return 1;
}

int handle_virtio_mmio_set_status_flag(virtio_emul_handler_t *emul_handler, uint32_t reg)
{
    int success = 1;

    // we only care about the new status
    emul_handler->data.Status &= reg;
    reg ^= emul_handler->data.Status;
    // printf("VIRTIO MMIO|INFO: set status flag 0x%x.\n", reg);

    switch (reg) {
        case VIRTIO_CONFIG_S_RESET:
            emul_handler->data.Status = 0;
            emul_handler->funs->device_reset(emul_handler);
            break;

        case VIRTIO_CONFIG_S_ACKNOWLEDGE:
            // are we following the initialization protocol?
            if (emul_handler->data.Status == 0) {
                emul_handler->data.Status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
                // nothing to do from our side (as the virtio backend).
            }
            break;

        case VIRTIO_CONFIG_S_DRIVER:
            // are we following the initialization protocol?
            if (emul_handler->data.Status && VIRTIO_CONFIG_S_ACKNOWLEDGE) {
                emul_handler->data.Status |= VIRTIO_CONFIG_S_DRIVER;
                // nothing to do from our side (as the virtio backend).
            }
            break;

        case VIRTIO_CONFIG_S_FEATURES_OK:
            // are we following the initialization protocol?
            if (emul_handler->data.Status && VIRTIO_CONFIG_S_DRIVER) {
                // are features OK?
                emul_handler->data.Status |= (emul_handler->data.features_happy ? VIRTIO_CONFIG_S_FEATURES_OK : 0);
            }
            break;

        case VIRTIO_CONFIG_S_DRIVER_OK:
            emul_handler->data.Status |= VIRTIO_CONFIG_S_DRIVER_OK;
            // probably do some san checks here
            break;

        case VIRTIO_CONFIG_S_FAILED:
            printf("VIRTIO MMIO|INFO: received FAILED status from driver, giving up this backend.\n");
            break;

        default:
            printf("VIRTIO MMIO|INFO: unknown device status 0x%x.\n", reg);
            success = 0;
    }
    return success;
}

static bool handle_virtio_mmio_reg_read(size_t vcpu_id, uint64_t fault_addr, uint64_t fsr, seL4_UserContext *regs)
{

    uint32_t reg = 0;
    int success = 1;
    // Need to find out which device it is when calculating the offset
    uint32_t offset = fault_addr - get_device_offset(fault_addr);
    // printf("\"%s\"|VIRTIO MMIO|INFO: Read from 0x%x.\n", sel4cp_name, offset);

    virtio_emul_handler_t *emul_handler = get_emul_handler_by_address(fault_addr);
    assert(emul_handler);

    switch (offset) {
        case REG_RANGE(REG_VIRTIO_MMIO_MAGIC_VALUE, REG_VIRTIO_MMIO_VERSION):
            reg = VIRTIO_MMIO_DEV_MAGIC;
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_VERSION, REG_VIRTIO_MMIO_DEVICE_ID):
            reg = VIRTIO_MMIO_DEV_VERSION;
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_DEVICE_ID, REG_VIRTIO_MMIO_VENDOR_ID):
            reg = emul_handler->data.DeviceID;
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_VENDOR_ID, REG_VIRTIO_MMIO_DEVICE_FEATURES):
            reg = emul_handler->data.VendorID;
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_DEVICE_FEATURES, REG_VIRTIO_MMIO_DEVICE_FEATURES_SEL):
            success = emul_handler->funs->get_device_features(emul_handler, &reg);
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_QUEUE_NUM_MAX, REG_VIRTIO_MMIO_QUEUE_NUM):
            reg = QUEUE_SIZE;
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_QUEUE_READY, REG_VIRTIO_MMIO_QUEUE_NOTIFY):
            if (emul_handler->data.QueueSel < VIRTIO_MMIO_NET_NUM_VIRTQUEUE) {
                reg = emul_handler->vqs[emul_handler->data.QueueSel].ready;
            }
            // simply ignore wrong queue number
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_INTERRUPT_STATUS, REG_VIRTIO_MMIO_INTERRUPT_ACK):
            reg = emul_handler->data.InterruptStatus;
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_STATUS, REG_VIRTIO_MMIO_QUEUE_DESC_LOW):
            success = handle_virtio_mmio_get_status_flag(emul_handler, &reg);
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_SHM_LEN_LOW, REG_VIRTIO_MMIO_SHM_LEN_HIGH):
            // To disable support for shared memory region (SHM) set the length to -1 (for both low and high registers). 
            // Reference: https://patchew.org/QEMU/20201220163539.2255963-1-laurent@vivier.eu/
            reg = (uint32_t)-1;
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_SHM_LEN_HIGH, REG_VIRTIO_MMIO_SHM_BASE_LOW):
            reg = (uint32_t)-1;
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_SHM_BASE_LOW, REG_VIRTIO_MMIO_SHM_BASE_HIGH):
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_SHM_BASE_HIGH, REG_VIRTIO_MMIO_QUEUE_RESET):
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_QUEUE_RESET, REG_VIRTIO_MMIO_CONFIG_GENERATION):
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_CONFIG_GENERATION, REG_VIRTIO_MMIO_CONFIG):
            /* ConfigGeneration will need to be update every time when the backend changes any of
             * the device config. Currently we only have virtio net backend that doesn't do any update
             * on the device config, so the ConfigGeneration is always 0. I left this comment
             * here as a reminder. */
            // reg = emul_handler->data.ConfigGeneration;
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_CONFIG, REG_VIRTIO_MMIO_CONFIG + 0x100):
            success = emul_handler->funs->get_device_config(emul_handler, offset, &reg);

            // uint32_t mask = fault_get_data_mask(fault_addr, fsr);
            // printf("\"%s\"|VIRTIO MMIO|INFO: device config offset 0x%x, value 0x%x, mask 0x%x\n", sel4cp_name, offset, reg & mask, mask);
            break;

        default:
            printf("VIRTIO MMIO|INFO: unknown register 0x%x.\n", offset);
            success = 0;
    }

    // we are expected to reply even on error
    uint32_t mask = fault_get_data_mask(fault_addr, fsr);
    int ret = fault_advance(vcpu_id, regs, fault_addr, fsr, reg & mask);
    assert(ret);

    return success;
}

static bool handle_virtio_mmio_reg_write(size_t vcpu_id, uint64_t fault_addr, uint64_t fsr, seL4_UserContext *regs)
{

    bool success = true;
    // Need to find out which device it is when calculating the offset
    uint32_t offset = fault_addr - get_device_offset(fault_addr);
    uint32_t data = fault_get_data(regs, fsr);
    uint32_t mask = fault_get_data_mask(fault_addr, fsr);
    /* Mask the data to write */
    data &= mask;

    // printf("\"%s\"|VIRTIO MMIO|INFO: Write to 0x%x.\n", sel4cp_name, offset);

    virtio_emul_handler_t *emul_handler = get_emul_handler_by_address(fault_addr);
    assert(emul_handler);

    switch (offset) {
        case REG_RANGE(REG_VIRTIO_MMIO_DEVICE_FEATURES_SEL, REG_VIRTIO_MMIO_DRIVER_FEATURES):
            emul_handler->data.DeviceFeaturesSel = data;
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_DRIVER_FEATURES, REG_VIRTIO_MMIO_DRIVER_FEATURES_SEL):
            success = emul_handler->funs->set_driver_features(emul_handler, data);
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_DRIVER_FEATURES_SEL, REG_VIRTIO_MMIO_QUEUE_SEL):
            emul_handler->data.DriverFeaturesSel = data;
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_QUEUE_SEL, REG_VIRTIO_MMIO_QUEUE_NUM_MAX):
            emul_handler->data.QueueSel = data;
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_QUEUE_NUM, REG_VIRTIO_MMIO_QUEUE_READY):
            if (emul_handler->data.QueueSel < VIRTIO_MMIO_NET_NUM_VIRTQUEUE) {
                struct vring *vring = get_current_vring_by_handler(emul_handler);
                vring->num = (unsigned int)data;
            }
            // simply ignore wrong queue number
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_QUEUE_READY, REG_VIRTIO_MMIO_QUEUE_NOTIFY):
            if (data == 0x1) {
                emul_handler->vqs[emul_handler->data.QueueSel].ready = true;
                // the vring is already in ram so we don't need to do any initiation
            }
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_QUEUE_NOTIFY, REG_VIRTIO_MMIO_INTERRUPT_STATUS):
            success = emul_handler->funs->queue_notify(emul_handler);
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_INTERRUPT_ACK, REG_VIRTIO_MMIO_STATUS):
            emul_handler->data.InterruptStatus &= ~data;
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_STATUS, REG_VIRTIO_MMIO_QUEUE_DESC_LOW):
            // printf("DATA IS 0x%x\n", data);
            success = handle_virtio_mmio_set_status_flag(emul_handler, data);
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_QUEUE_DESC_LOW, REG_VIRTIO_MMIO_QUEUE_DESC_HIGH):
            if (emul_handler->data.QueueSel < VIRTIO_MMIO_NET_NUM_VIRTQUEUE) {
                struct vring *vring = get_current_vring_by_handler(emul_handler);
                uint64_t ptr = (uint64_t)vring->desc;
                ptr |= data;
                vring->desc = (struct vring_desc *)ptr;
            }
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_QUEUE_DESC_HIGH, REG_VIRTIO_MMIO_QUEUE_AVAIL_LOW):
            if (emul_handler->data.QueueSel < VIRTIO_MMIO_NET_NUM_VIRTQUEUE) {
                struct vring *vring = get_current_vring_by_handler(emul_handler);
                uint64_t ptr = (uint64_t)vring->desc;
                ptr |= (uint64_t)data << 32;
                vring->desc = (struct vring_desc *)ptr;
            }
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_QUEUE_AVAIL_LOW, REG_VIRTIO_MMIO_QUEUE_AVAIL_HIGH):
            if (emul_handler->data.QueueSel < VIRTIO_MMIO_NET_NUM_VIRTQUEUE) {
                struct vring *vring = get_current_vring_by_handler(emul_handler);
                uint64_t ptr = (uint64_t)vring->avail;
                ptr |= data;
                vring->avail = (struct vring_avail *)ptr;
            }
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_QUEUE_AVAIL_HIGH, REG_VIRTIO_MMIO_QUEUE_USED_LOW):
            if (emul_handler->data.QueueSel < VIRTIO_MMIO_NET_NUM_VIRTQUEUE) {
                struct vring *vring = get_current_vring_by_handler(emul_handler);
                uint64_t ptr = (uint64_t)vring->avail;
                ptr |= (uint64_t)data << 32;
                vring->avail = (struct vring_avail *)ptr;
                // printf("VIRTIO MMIO|INFO: vring avail 0x%lx\n.", ptr);
            }
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_QUEUE_USED_LOW, REG_VIRTIO_MMIO_QUEUE_USED_HIGH):
            if (emul_handler->data.QueueSel < VIRTIO_MMIO_NET_NUM_VIRTQUEUE) {
                struct vring *vring = get_current_vring_by_handler(emul_handler);
                uint64_t ptr = (uint64_t)vring->used;
                ptr |= data;
                vring->used = (struct vring_used *)ptr;
            }
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_QUEUE_USED_HIGH, REG_VIRTIO_MMIO_SHM_SEL):
            if (emul_handler->data.QueueSel < VIRTIO_MMIO_NET_NUM_VIRTQUEUE) {
                struct vring *vring = get_current_vring_by_handler(emul_handler);
                uint64_t ptr = (uint64_t)vring->used;
                ptr |= (uint64_t)data << 32;
                vring->used = (struct vring_used *)ptr;
                // printf("VIRTIO MMIO|INFO: vring used 0x%lx\n.", ptr);
            }
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_SHM_SEL, REG_VIRTIO_MMIO_SHM_LEN_LOW):
            // printf("VIRTIO MMIO|INFO: SHM_SEL is 0x%x.\n", data);
            // Not supporting SHM
            break;
        
        case REG_RANGE(REG_VIRTIO_MMIO_QUEUE_RESET, REG_VIRTIO_MMIO_CONFIG_GENERATION):
            // Not sure what this is needed for yet.
            break;

        case REG_RANGE(REG_VIRTIO_MMIO_CONFIG, REG_VIRTIO_MMIO_CONFIG + 0x100):
            success = emul_handler->funs->set_device_config(emul_handler, offset, data);
            break;
        default:
            printf("VIRTIO MMIO|INFO: unknown register 0x%x.", offset);
            success = false;
    }

    int ret = fault_advance_vcpu(vcpu_id, regs);
    assert(ret);

    return success;
}

bool handle_virtio_mmio_fault(size_t vcpu_id, uint64_t fault_addr, uint64_t fsr, seL4_UserContext *regs)
{
    assert(fault_addr >= VIRTIO_ADDRESS_START);
    assert(fault_addr < VIRTIO_ADDRESS_END);

    if (!get_emul_handler_by_address(fault_addr)) {
        printf("\"%s\"|VIRTIO MMIO|INFO: no virtio backend registered for fault address 0x%x.\n", sel4cp_name, fault_addr);
        return false;
    }

    if (fault_is_read(fsr)) {
        return handle_virtio_mmio_reg_read(vcpu_id, fault_addr, fsr, regs);
    } else {
        return handle_virtio_mmio_reg_write(vcpu_id, fault_addr, fsr, regs);
    }
}
