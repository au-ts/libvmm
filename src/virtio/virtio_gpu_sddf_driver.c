/*
 * Copyright 2023, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include "../util/util.h"
#include "include/config/virtio_gpu.h"
#include "virtio_mmio.h"
#include "virtio_gpu_emul.h"
#include "virtio_gpu_device.h"
#include "virtio_gpu_sddf_driver.h"


void gpu_virtqueue_to_sddf(uio_map_t *uio_map, virtqueue_t *vq) {
    struct vring *vring = &vq->vring;

    /* read the current guest index */
    uint16_t guest_idx = vring->avail->idx;
    uint16_t idx = vq->last_idx;

    // printf("\"%s\"|VIRTIO GPU|INFO: ------------- Driver notified device -------------\n", sel4cp_name);
    
    for (; idx != guest_idx; idx++) {
        uint16_t desc_head = vring->avail->ring[idx % vring->num];

        uint16_t curr_desc_head = desc_head;

        // printf("\"%s\"|VIRTIO GPU|INFO: Descriptor index is %d, Descriptor flags are: 0x%x, length is 0x%x\n", sel4cp_name, curr_desc_head, (uint16_t)vring->desc[curr_desc_head].flags, vring->desc[curr_desc_head].len);

        // Print out what the command type is
        struct virtio_gpu_ctrl_hdr *header = (void *)vring->desc[curr_desc_head].addr;
        printf("\"%s\"|VIRTIO GPU|INFO: ----- Buffer header is 0x%x -----\n", sel4cp_name, header->type);
        
        // Parse different commands
        switch (header->type) {
            case VIRTIO_GPU_CMD_RESOURCE_CREATE_2D: {
                // struct virtio_gpu_resource_create_2d *request = (void *)vring->desc[curr_desc_head].addr;
                // printf("\"%s\"|VIRTIO GPU|INFO: initialised resource ID %d\n", sel4cp_name, request->resource_id);
                break;
            }
            case VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING: { // This chain has two descriptors, the first one is header, second one is mem_entries 
                // struct virtio_gpu_resource_attach_backing *request = (void *)vring->desc[curr_desc_head].addr;
                // printf("\"%s\"|VIRTIO GPU|INFO: number of guest pages for backing is %d\n", sel4cp_name, request->nr_entries);
                // printf("\"%s\"|VIRTIO GPU|INFO: attaching resource ID %d\n", sel4cp_name, request->resource_id);

                curr_desc_head = vring->desc[curr_desc_head].next;
                // printf("\"%s\"|VIRTIO GPU|INFO: Descriptor index is %d, Descriptor flags are: 0x%x, length is 0x%x\n", sel4cp_name, curr_desc_head, (uint16_t)vring->desc[curr_desc_head].flags, vring->desc[curr_desc_head].len);

                struct virtio_gpu_mem_entry *mem_entries = (void *)vring->desc[curr_desc_head].addr;
                printf("\"%s\"|VIRTIO GPU|INFO: address of memory entry is 0x%x\n", sel4cp_name, mem_entries[0].addr);
                printf("\"%s\"|VIRTIO GPU|INFO: length of memory entry is %d\n", sel4cp_name, mem_entries[0].length);

                // memcpy((void *)uio_map0, (void *)mem_entries[0].addr, mem_entries[0].length);
                // // Check whats inside the memory                
                // for (int i = 0; i < mem_entries[0].length; i++) {
                //     printf("%x", (uint8_t *)mem_entries[0].addr + i);
                // }
                break;
            }
        }
        
        // // This loop brings curr_desc_head to the final descriptor in the chain, if it is not already at it,
        // // which is where you write the response from device back to driver.
        // do {
        //     // printf("\"%s\"|VIRTIO GPU|INFO: Descriptor index is %d, Descriptor flags are: 0x%x, length is 0x%x\n", sel4cp_name, curr_desc_head, (uint16_t)vring->desc[curr_desc_head].flags, vring->desc[curr_desc_head].len);
        //     curr_desc_head = vring->desc[curr_desc_head].next;
        // } while (vring->desc[curr_desc_head].flags & VRING_DESC_F_NEXT);

        // // Return the proper response to the driver
        // switch (header->type) {
        //     case VIRTIO_GPU_CMD_GET_DISPLAY_INFO: {
        //         struct virtio_gpu_resp_display_info *display_info = (void *)vring->desc[curr_desc_head].addr;
                
        //         display_info->hdr.type = VIRTIO_GPU_RESP_OK_DISPLAY_INFO;
                
        //         display_info->pmodes[0].r.width = 1024; // vu7a+ screen size
        //         display_info->pmodes[0].r.height = 600;
        //         display_info->pmodes[0].r.x = 0;
        //         display_info->pmodes[0].r.y = 0;

        //         display_info->pmodes[0].enabled = 1;

        //         display_info->pmodes[0].flags = 0; // what even is this? spec does not say.
        //         break;
        //     }
        //     // case VIRTIO_GPU_CMD_GET_CAPSET: {
        //     //     struct virtio_gpu_resp_capset_info *capset_info = (void *)vring->desc[curr_desc_head].addr;
                
        //     //     capset_info->hdr.type = VIRTIO_GPU_RESP_OK_CAPSET_INFO;

        //     //     capset_info->capset_id = 
        //     //     break;
        //     // }
        //     default: {
        //         struct virtio_gpu_ctrl_hdr *response_header = (void *)vring->desc[curr_desc_head].addr;
        //         response_header->type = VIRTIO_GPU_RESP_OK_NODATA;
        //         break;
        //     }
        // }
    }

    vq->last_idx = idx;
}

void gpu_sddf_to_vqueue(virtqueue_t vq, uio_map_t uio_map) {

}