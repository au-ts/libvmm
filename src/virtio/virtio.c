/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include "fault.h"
#include "util/util.h"
#include "virtio/console.h"
#include "virtio/block.h"
#include "virtio/virtio.h"
#include "virq.h"
#include "serial/libserialsharedringbuffer/include/shared_ringbuffer.h"
#include "block/libblocksharedringbuffer/include/blk_shared_ringbuffer.h"

static struct virtio_queue_handler virtio_console_queues[VIRTIO_CONSOLE_NUM_VIRTQ];
static struct virtio_queue_handler virtio_blk_queues[VIRTIO_BLK_NUM_VIRTQ];

void virtio_virq_default_ack(size_t vcpu_id, int irq, void *cookie) {
    // nothing needs to be done
}

// assumes virq controller has been initialised
bool virtio_mmio_device_init(virtio_device_t *dev,
                            enum virtio_device_type type,
                            uintptr_t region_base,
                            uintptr_t region_size,
                            size_t virq,
                            void **sddf_rings,
                            size_t sddf_ch)
{
    bool success = true;
    switch (type) {
        case CONSOLE:
            virtio_console_init(dev, virtio_console_queues, VIRTIO_CONSOLE_NUM_VIRTQ, virq, sddf_rings, sddf_ch);
            success = fault_register_vm_exception_handler(region_base,
                                                        region_size,
                                                        &virtio_mmio_fault_handle,
                                                        dev);
            break;
        case BLK:
            // @ericc: Ideally we would call the init like this:
            // virtio_blk_init(dev, virtio_blk_queues, VIRTIO_BLK_NUM_VIRTQ, virq, sddf_ring, sddf_mux_ch)
            virtio_blk_init(dev, virtio_blk_queues, VIRTIO_BLK_NUM_VIRTQ, virq, sddf_rings, sddf_ch);
            success = fault_register_vm_exception_handler(region_base,
                                                        region_size,
                                                        &virtio_mmio_fault_handle,
                                                        dev);
            break;
        default:
            LOG_VMM_ERR("Unsupported virtIO device type given: %d\n", type);
            return false;
        }

    if (!success) {
        LOG_VMM_ERR("Could not register virtual memory fault handler for "
                    "virtIO region [0x%lx..0x%lx)\n", region_base, region_base + region_size);
        return false;
    }

    /* Register the virtual IRQ that will be used to communicate from the device
     * to the guest. This assumes that the interrupt controller is already setup. */
    // @ivanv: we should check that (on AArch64) the virq is an SPI.
    success = virq_register(GUEST_VCPU_ID, virq, &virtio_virq_default_ack, NULL);
    assert(success);

    return success;
}
