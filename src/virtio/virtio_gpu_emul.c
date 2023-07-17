/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stddef.h>
#include "virtio_mmio.h"
#include "virtio_gpu_emul.h"
#include "include/config/virtio_gpu.h"
#include "include/config/virtio_config.h"
#include "../vgic/vgic.h"

// virtio gpu mmio emul interface

// @jade, @ivanv: need to be able to get it from vgic
#define VCPU_ID 0

#define BIT_LOW(n) (1ul<<(n))
#define BIT_HIGH(n) (1ul<<(n - 32 ))

#define CONTROL_QUEUE 0
#define CURSOR_QUEUE 1

#define BUF_SIZE 0x1000

#define REG_RANGE(r0, r1)   r0 ... (r1 - 1)

// emul handler for an instance of virtio gpu
static virtio_emul_handler_t gpu_emul_handler;

// the list of virtqueue handlers for an instance of virtio gpu
static virtqueue_t vqs[VIRTIO_MMIO_GPU_NUM_VIRTQUEUE];

static struct virtio_gpu_config gpu_config;

void virtio_gpu_ack(uint64_t vcpu_id, int irq, void *cookie) {
    printf("\"%s\"|VIRTIO GPU|INFO: virtio_gpu_ack %d\n", sel4cp_name, irq);
    // sel4cp_irq_ack(vcpu_id, irq);
}

virtio_emul_handler_t *get_virtio_gpu_emul_handler(void)
{
    // san check in case somebody wants to get the handler of an uninitialised backend
    if (gpu_emul_handler.data.VendorID != VIRTIO_MMIO_DEV_VENDOR_ID) {
        return NULL;
    }

    return &gpu_emul_handler;
}

static void virtio_gpu_emul_reset(virtio_emul_handler_t *self)
{
    printf("\"%s\"|VIRTIO GPU|INFO: device has been reset\n", sel4cp_name);
    
    self->data.Status = 0;
    
    vqs[CONTROL_QUEUE].last_idx = 0;
    vqs[CONTROL_QUEUE].ready = 0;

    vqs[CURSOR_QUEUE].last_idx = 0;
    vqs[CURSOR_QUEUE].ready = 0;
}

static int virtio_gpu_emul_get_device_features(virtio_emul_handler_t *self, uint32_t *features)
{
    if (self->data.Status & VIRTIO_CONFIG_S_FEATURES_OK) {
        printf("VIRTIO GPU|WARNING: driver somehow wants to read device features after FEATURES_OK\n");
    }

    switch (self->data.DeviceFeaturesSel) {
        // feature bits 0 to 31
        case 0:
            // No GPU specific features supported
            break;
        // features bits 32 to 63
        case 1:
            *features = BIT_HIGH(VIRTIO_F_VERSION_1);
            break;
        default:
            printf("VIRTIO GPU|INFO: driver sets DeviceFeaturesSel to 0x%x, which doesn't make sense\n", self->data.DeviceFeaturesSel);
            return 0;
    }
    return 1;
}

static int virtio_gpu_emul_set_driver_features(virtio_emul_handler_t *self, uint32_t features)
{
    int success = 1;

    switch (self->data.DriverFeaturesSel) {
        // feature bits 0 to 31
        case 0:
            // The device initialisation protocol says the driver should read device feature bits,
            // and write the subset of feature bits understood by the OS and driver to the device.
            // But no GPU specific features supported.
            break;

        // features bits 32 to 63
        case 1:
            success = (features == BIT_HIGH(VIRTIO_F_VERSION_1));
            break;

        default:
            printf("VIRTIO GPU|INFO: driver sets DriverFeaturesSel to 0x%x, which doesn't make sense\n", self->data.DriverFeaturesSel);
            success = 0;
    }
    if (success) {
        self->data.features_happy = 1;
    }
    return success;
}

static int virtio_gpu_emul_get_device_config(struct virtio_emul_handler *self, uint32_t offset, uint32_t *ret_val)
{
    void * config_base_addr = (void *)&gpu_config;
    uint32_t *config_field_addr = (uint32_t *)(config_base_addr + (offset - REG_VIRTIO_MMIO_CONFIG));
    *ret_val = *config_field_addr;
    // printf("VIRTIO GPU|INFO: get_device_config_field config_field_address 0x%x returns retval %d\n", config_field_addr, *ret_val);
    return 1;
}

static int virtio_gpu_emul_set_device_config(struct virtio_emul_handler *self, uint32_t offset, uint32_t val)
{
    void * config_base_addr = (void *)&gpu_config;
    uint32_t *config_field_addr = (uint32_t *)(config_base_addr + (offset - REG_VIRTIO_MMIO_CONFIG));
    *config_field_addr = val;
    // printf("VIRTIO GPU|INFO: set_device_config_field set 0x%x to %d\n", config_field_addr, val);
    return 1;
}

// notify the guest that their avail buffer has been used
static void virtio_gpu_emul_handle_used_buffer_notif(struct virtio_emul_handler *self, uint16_t desc_head)
{
    // set IRQ reason as used buffer notification (set bit 0 to 1)
    self->data.InterruptStatus = BIT_LOW(0);

    // add to used ring
    struct vring *vring = &vqs[CONTROL_QUEUE].vring;

    struct vring_used_elem used_elem = {desc_head, 0};
    uint16_t guest_idx = vring->used->idx;

    vring->used->ring[guest_idx % vring->num] = used_elem;
    vring->used->idx++;
}


static int virtio_gpu_emul_handle_queue_notify(struct virtio_emul_handler *self)
{
    struct vring *vring = &vqs[CONTROL_QUEUE].vring;

    /* read the current guest index */
    uint16_t guest_idx = vring->avail->idx;
    uint16_t idx = vqs[CONTROL_QUEUE].last_idx;

    printf("\"%s\"|VIRTIO GPU|INFO: ------------- Driver notified device -------------\n", sel4cp_name);
    
    for (; idx != guest_idx; idx++) {
        uint16_t desc_head = vring->avail->ring[idx % vring->num];

        uint16_t curr_desc_head = desc_head;

        // Print out what the command type is
        struct virtio_gpu_ctrl_hdr *header = (void *)vring->desc[curr_desc_head].addr;
        printf("\"%s\"|VIRTIO GPU|INFO: >>>>>>>>>> Buffer header is 0x%x <<<<<<<<<<\n", sel4cp_name, header->type);
        
        // Parse different commands
        switch (header->type) {
            case VIRTIO_GPU_CMD_RESOURCE_CREATE_2D: {
                struct virtio_gpu_resource_create_2d *request = (void *)vring->desc[curr_desc_head].addr;
                printf("\"%s\"|VIRTIO GPU|INFO: initialised resource ID %d\n", sel4cp_name, request->resource_id);
                break;
            }
            case VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING: { // This chain has two descriptors, the first one is header, second one is mem_entries 
                struct virtio_gpu_resource_attach_backing *request = (void *)vring->desc[curr_desc_head].addr;
                printf("\"%s\"|VIRTIO GPU|INFO: number of guest pages for backing is %d\n", sel4cp_name, request->nr_entries);
                printf("\"%s\"|VIRTIO GPU|INFO: attaching resource ID %d\n", sel4cp_name, request->resource_id);

                printf("\"%s\"|VIRTIO GPU|INFO: Descriptor index is %d, Descriptor flags are: 0x%x, length is 0x%x\n", sel4cp_name, curr_desc_head, (uint16_t)vring->desc[curr_desc_head].flags, vring->desc[curr_desc_head].len);
                curr_desc_head = vring->desc[curr_desc_head].next;

                struct virtio_gpu_mem_entry *mem_entries = (void *)vring->desc[curr_desc_head].addr;
                printf("\"%s\"|VIRTIO GPU|INFO: address of memory entry is 0x%x\n", sel4cp_name, mem_entries[0].addr);
                printf("\"%s\"|VIRTIO GPU|INFO: length of memory entry is %d\n", sel4cp_name, mem_entries[0].length);

                break;
            }
        }
        
        // This loop brings curr_desc_head to the final descriptor in the chain, if it is not already at it,
        // which is where you write the response from device back to driver.
        do {
            printf("\"%s\"|VIRTIO GPU|INFO: Descriptor index is %d, Descriptor flags are: 0x%x, length is 0x%x\n", sel4cp_name, curr_desc_head, (uint16_t)vring->desc[curr_desc_head].flags, vring->desc[curr_desc_head].len);
            curr_desc_head = vring->desc[curr_desc_head].next;
        } while (vring->desc[curr_desc_head].flags & VRING_DESC_F_NEXT);

        // Return the proper response to the driver
        switch (header->type) {
            case VIRTIO_GPU_CMD_GET_DISPLAY_INFO: {
                struct virtio_gpu_resp_display_info *display_info = (void *)vring->desc[curr_desc_head].addr;
                
                display_info->hdr.type = VIRTIO_GPU_RESP_OK_DISPLAY_INFO;
                
                display_info->pmodes[0].r.width = 1024; // vu7a+ screen size
                display_info->pmodes[0].r.height = 600;
                display_info->pmodes[0].r.x = 0;
                display_info->pmodes[0].r.y = 0;

                display_info->pmodes[0].enabled = 1;

                display_info->pmodes[0].flags = 0; // what even is this? spec does not say.
                break;
            }
            // case VIRTIO_GPU_CMD_GET_CAPSET: {
            //     struct virtio_gpu_resp_capset_info *capset_info = (void *)vring->desc[curr_desc_head].addr;
                
            //     capset_info->hdr.type = VIRTIO_GPU_RESP_OK_CAPSET_INFO;

            //     capset_info->capset_id = 
            //     break;
            // }
            default: {
                struct virtio_gpu_ctrl_hdr *response_header = (void *)vring->desc[curr_desc_head].addr;
                response_header->type = VIRTIO_GPU_RESP_OK_NODATA;
                break;
            }
        }

        // Will need to notify host VM eventually, somewhere
        // sel4cp_notify(VIRTIO_GPU_CH);

        virtio_gpu_emul_handle_used_buffer_notif(self, desc_head);
    }
    
    bool success = vgic_inject_irq(VCPU_ID, VIRTIO_GPU_IRQ);
    // we can't inject irqs?? panic.
    assert(success);

    vqs[CONTROL_QUEUE].last_idx = idx;
    return 1;
}

static virtio_emul_funs_t gpu_emul_funs = {
    .device_reset = virtio_gpu_emul_reset,
    .get_device_features = virtio_gpu_emul_get_device_features,
    .set_driver_features = virtio_gpu_emul_set_driver_features,
    .get_device_config = virtio_gpu_emul_get_device_config,
    .set_device_config = virtio_gpu_emul_set_device_config,
    .queue_notify = virtio_gpu_emul_handle_queue_notify,
};

static void virtio_gpu_config_init(void) {
    // Hard coded values, need to retrieve these from device.
    gpu_config.events_read = 0;
    gpu_config.events_clear = 0;
    gpu_config.num_scanouts = 1;
    gpu_config.num_capsets = 0;
}

void virtio_gpu_emul_init(void)
{
    virtio_gpu_config_init();
    
    gpu_emul_handler.data.DeviceID = DEVICE_ID_VIRTIO_GPU;
    gpu_emul_handler.data.VendorID = VIRTIO_MMIO_DEV_VENDOR_ID;
    gpu_emul_handler.funs = &gpu_emul_funs;

    gpu_emul_handler.vqs = vqs;
}