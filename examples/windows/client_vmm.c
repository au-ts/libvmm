/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <microkit.h>
#include <libvmm/libvmm.h>
#include <sddf/serial/queue.h>
#include <sddf/serial/config.h>
#include <sddf/blk/queue.h>
#include <sddf/blk/config.h>
#include <sddf/network/queue.h>
#include <sddf/network/config.h>
#include <sddf/timer/config.h>
#include <sddf/timer/client.h>
#include <sddf/util/printf.h>
#include "guest_arch_init.h"

/* Data from sdfgen */
__attribute__((__section__(".timer_client_config"))) timer_client_config_t timer_config;
__attribute__((__section__(".blk_client_config"))) blk_client_config_t blk_config;
__attribute__((__section__(".net_client_config"))) net_client_config_t net_config;
__attribute__((__section__(".vmm_config"))) vmm_config_t vmm_config;

blk_queue_handle_t blk_queue;

net_queue_handle_t net_rx_queue;
net_queue_handle_t net_tx_queue;

/* Bookkeeping structures for virtio devices */
struct virtio_blk_device virtio_blk;
struct virtio_net_device virtio_net;

void init(void)
{
    assert(blk_config_check_magic(&blk_config));
    assert(vmm_config_check_magic(&vmm_config));
    assert(net_config_check_magic(&net_config));

    /* Initialise the VMM and the VCPU */
    LOG_VMM("starting \"%s\"\n", microkit_name);

    bool success = guest_arch_init();
    if (!success) {
        LOG_VMM_ERR("Failed to initialise guest\n");
        return;
    }

    /* Initialise sDDF Block subsystem. */
    blk_queue_init(&blk_queue, blk_config.virt.req_queue.vaddr, blk_config.virt.resp_queue.vaddr,
                   blk_config.virt.num_buffers);
    /* Busy wait until blk device is ready */
    blk_storage_info_t *storage_info = blk_config.virt.storage_info.vaddr;
    while (!blk_storage_is_ready(storage_info));

    /* Initialise sDDF Network subsystem. */
    net_queue_init(&net_rx_queue, net_config.rx.free_queue.vaddr, net_config.rx.active_queue.vaddr,
                   net_config.rx.num_buffers);
    net_queue_init(&net_tx_queue, net_config.tx.free_queue.vaddr, net_config.tx.active_queue.vaddr,
                   net_config.tx.num_buffers);
    net_buffers_init(&net_tx_queue, 0);

    if (!virtio_arch_init()) {
        LOG_VMM_ERR("Failed to initialise virtIO devices\n");
        return;
    }

    /* Finally start the guest */
    if (!guest_arch_start()) {
        LOG_VMM_ERR("Failed to start guest\n");
        return;
    }
    LOG_VMM("%s is ready\n", microkit_name);
}

#define HOST_FB_VADDR 0x4000000000

void notified(microkit_channel ch)
{
    if (ch == blk_config.virt.id) {
        virtio_blk_handle_resp(&virtio_blk);
    } else if (ch == net_config.rx.id) {
        virtio_net_handle_rx(&virtio_net);
    } else if (ch == net_config.tx.id) {
        /* Nothing to do */
    } else if (ch == timer_config.driver_id) {
        memcpy((void *) HOST_FB_VADDR, gpa_to_hva(0xCEE55000, 1), 640 * 480 * 4);
        /* fb refresh tick */
        sddf_timer_set_timeout(timer_config.driver_id, NS_IN_S / 10);
    } else {
        virq_handle_passthrough(ch);
    }
}

seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo)
{
    bool success = fault_handle(child, msginfo);
    if (success) {
        /* Now that we have handled the fault successfully, we reply to it so
         * that the guest can resume execution. */
        *reply_msginfo = microkit_msginfo_new(0, 0);
        return seL4_True;
    }

    return seL4_False;
}
