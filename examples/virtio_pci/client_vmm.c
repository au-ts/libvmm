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
#include <sddf/util/printf.h>
#include "guest_arch_init.h"

/* Data from sdfgen */
__attribute__((__section__(".serial_client_config"))) serial_client_config_t serial_config;
__attribute__((__section__(".blk_client_config"))) blk_client_config_t blk_config;
__attribute__((__section__(".net_client_config"))) net_client_config_t net_config;
__attribute__((__section__(".vmm_config"))) vmm_config_t vmm_config;

/* sDDF data */
serial_queue_handle_t serial_rx_queue;
serial_queue_handle_t serial_tx_queue;

blk_queue_handle_t blk_queue;

net_queue_handle_t net_rx_queue;
net_queue_handle_t net_tx_queue;

/* Bookkeeping structures for virtio devices */
struct virtio_console_device virtio_console;
struct virtio_blk_device virtio_blk;
struct virtio_net_device virtio_net;

void init(void)
{
    assert(serial_config_check_magic(&serial_config));
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

    /* Initialise sDDF Serial subsystem. */
    serial_queue_init(&serial_rx_queue, serial_config.rx.queue.vaddr, serial_config.rx.data.size,
                      serial_config.rx.data.vaddr);
    serial_queue_init(&serial_tx_queue, serial_config.tx.queue.vaddr, serial_config.tx.data.size,
                      serial_config.tx.data.vaddr);

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

void notified(microkit_channel ch)
{
    if (ch == serial_config.rx.id) {
        virtio_console_queue_notify(&virtio_console);
    } else if (ch == serial_config.tx.id || ch == net_config.tx.id) {
        /* Nothing to do */
    } else if (ch == blk_config.virt.id) {
        virtio_blk_handle_resp(&virtio_blk);
    } else if (ch == net_config.rx.id) {
        virtio_net_handle_rx(&virtio_net);
    } else {
        LOG_VMM_ERR("Unexpected channel, ch: 0x%x\n", ch);
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
