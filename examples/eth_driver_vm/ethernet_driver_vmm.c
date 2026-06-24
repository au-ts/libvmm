/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <microkit.h>
#include <libvmm/config.h>
#include <libvmm/guest.h>
#include <libvmm/virq.h>
#include <libvmm/util/util.h>
#include <libvmm/virtio/virtio.h>
#include <libvmm/virtio/console.h>
#include <libvmm/arch/aarch64/linux.h>
#include <libvmm/arch/aarch64/fault.h>
#include <sddf/serial/queue.h>
#include <sddf/serial/config.h>
#include <sddf/network/queue.h>
#include <sddf/network/config.h>
#include <sddf/util/printf.h>

#include <uio/net.h>
#include <expect.h>

__attribute__((__section__(".serial_client_config"))) serial_client_config_t serial_config;
__attribute__((__section__(".device_resources"))) device_resources_t device_resources;
__attribute__((__section__(".vmm_config"))) vmm_config_t vmm_config;

__attribute__((__section__(".net_driver_config"))) net_driver_config_t net_drv_config;
// A bit dubious but I'm not sure how to *cleanly* fix
__attribute__((__section__(".net_virt_rx_config"))) net_virt_rx_config_t net_rx_config;
__attribute__((__section__(".net_virt_tx_config"))) net_virt_tx_config_t net_tx_config;

/* Data for the guest's kernel image. */
extern char _guest_kernel_image[];
extern char _guest_kernel_image_end[];
/* Data for the device tree to be passed to the kernel. */
extern char _guest_dtb_image[];
extern char _guest_dtb_image_end[];
/* Data for the initial RAM disk to be passed to the kernel. */
extern char _guest_initrd_image[];
extern char _guest_initrd_image_end[];
/* Microkit will set this variable to the start of the guest RAM memory region. */
uintptr_t guest_ram_vaddr;

/* Virtio Console */
serial_queue_handle_t serial_rx_queue;
serial_queue_handle_t serial_tx_queue;

static struct virtio_console_device virtio_console;

#define PAGE_SIZE_4K 0x1000

int uio_rx_irq_num;
int uio_tx_irq_num;

void uio_net_to_vmm_ack(size_t vcpu_id, int irq, void *cookie)
{
}

bool uio_net_from_vmm_tx_signal(size_t vcpu_id, uintptr_t addr, size_t fsr, seL4_UserContext *regs, void *data)
{
    microkit_notify(net_drv_config.virt_tx.id);
    return true;
}
bool uio_net_from_vmm_rx_signal(size_t vcpu_id, uintptr_t addr, size_t fsr, seL4_UserContext *regs, void *data)
{
    microkit_notify(net_drv_config.virt_rx.id);
    return true;
}

void init(void)
{
    assert(serial_config_check_magic(&serial_config));
    assert(device_resources_check_magic(&device_resources));
    assert(vmm_config_check_magic(&vmm_config));
    assert(net_config_check_magic(&net_drv_config));
    assert(net_config_check_magic(&net_rx_config));
    assert(net_config_check_magic(&net_tx_config));

    /* Initialise the VMM and the VCPU */
    LOG_VMM("starting \"%s\"\n", microkit_name);
    /* Place all the binaries in the right locations before starting the guest */
    size_t kernel_size = _guest_kernel_image_end - _guest_kernel_image;
    size_t dtb_size = _guest_dtb_image_end - _guest_dtb_image;
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;
    uintptr_t kernel_pc = linux_setup_images(vmm_config.ram, (uintptr_t)_guest_kernel_image, kernel_size,
                                             (uintptr_t)_guest_dtb_image, vmm_config.dtb, dtb_size,
                                             (uintptr_t)_guest_initrd_image, vmm_config.initrd, initrd_size);
    if (!kernel_pc) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }

    /* Initialise the virtual GIC driver */
    bool success = virq_controller_init();
    if (!success) {
        LOG_VMM_ERR("Failed to initialise emulated interrupt controller\n");
        return;
    }

    /* Register pass-through IRQs: Ethernet MAC, PHY and Work IRQs */
    assert(vmm_config.num_irqs == NUM_EXPECTED_PASS_THRU_IRQS);
    for (int i = 0; i < vmm_config.num_irqs; i++) {
        if (!virq_register_passthrough(GUEST_BOOT_VCPU_ID, vmm_config.irqs[i].irq, vmm_config.irqs[i].id)) {
            LOG_VMM_ERR("Failed to pass through IRQ %d\n", vmm_config.irqs[i].irq);
            return;
        }
    }

    /* Find all the UIO nodes */
    assert(vmm_config.num_uio_regions == NUM_EXPECTED_UIO_REGIONS);
    int uio_data_passing_node_idx = -1;
    int uio_rx_fault_node_idx = -1;
    int uio_tx_fault_node_idx = -1;
    int uio_rx_irq_node_idx = -1;
    int uio_tx_irq_node_idx = -1;
    for (int i = 0; i < vmm_config.num_uio_regions; i++) {
        if (!strcmp(UIO_NET_DATA_PASSING_NAME, vmm_config.uios[i].name)) {
            uio_data_passing_node_idx = i;
        } else if (!strcmp(UIO_NET_RX_FAULT_NAME, vmm_config.uios[i].name)) {
            uio_rx_fault_node_idx = i;
        } else if (!strcmp(UIO_NET_TX_FAULT_NAME, vmm_config.uios[i].name)) {
            uio_tx_fault_node_idx = i;
        } else if (!strcmp(UIO_NET_RX_IRQ_NAME, vmm_config.uios[i].name)) {
            uio_rx_irq_node_idx = i;
        } else if (!strcmp(UIO_NET_TX_IRQ_NAME, vmm_config.uios[i].name)) {
            uio_tx_irq_node_idx = i;
        }
    }
    assert(uio_data_passing_node_idx != -1);
    assert(uio_rx_fault_node_idx != -1);
    assert(uio_tx_fault_node_idx != -1);
    assert(uio_rx_irq_node_idx != -1);
    assert(uio_tx_irq_node_idx != -1);

    uio_rx_irq_num = vmm_config.uios[uio_rx_irq_node_idx].irq;
    uio_tx_irq_num = vmm_config.uios[uio_tx_irq_node_idx].irq;

    /* Register VMM -> guest IRQs */
    if (!virq_register(GUEST_BOOT_VCPU_ID, uio_rx_irq_num, uio_net_to_vmm_ack, NULL)) {
        LOG_VMM_ERR("Failed to register UIO RX interrupt\n");
        return;
    } else {
        LOG_VMM("Register UIO RX interrupt number %d\n", uio_rx_irq_num);
    }
    if (!virq_register(GUEST_BOOT_VCPU_ID, uio_tx_irq_num, uio_net_to_vmm_ack, NULL)) {
        LOG_VMM_ERR("Failed to register UIO TX interrupt\n");
        return;
    } else {
        LOG_VMM("Register UIO TX interrupt number %d\n", uio_tx_irq_num);
    }

    /* Register guest -> VMM notification fault mechanism */
    if (!fault_register_vm_exception_handler(vmm_config.uios[uio_rx_fault_node_idx].guest_paddr, PAGE_SIZE_4K,
                                             &uio_net_from_vmm_rx_signal, NULL)) {
        LOG_VMM_ERR("Failed to register UIO RX fault handler\n");
        return;
    } else {
        LOG_VMM("Register UIO RX fault address 0x%p\n", (void *)vmm_config.uios[uio_rx_fault_node_idx].guest_paddr);
    }
    if (!fault_register_vm_exception_handler(vmm_config.uios[uio_tx_fault_node_idx].guest_paddr, PAGE_SIZE_4K,
                                             &uio_net_from_vmm_tx_signal, NULL)) {
        LOG_VMM_ERR("Failed to register UIO TX fault handler\n");
        return;
    } else {
        LOG_VMM("Register UIO TX fault address 0x%p\n", (void *)vmm_config.uios[uio_tx_fault_node_idx].guest_paddr);
    }

    /* Copy the net config struct into the data passing page so that the guest can see it. */
    net_drv_vm_data_passing_t *data_passing =
        (net_drv_vm_data_passing_t *)vmm_config.uios[uio_data_passing_node_idx].vmm_vaddr;
    memcpy(&data_passing->net_config, &net_drv_config, sizeof(net_driver_config_t));
    data_passing->rx_data_paddr = net_rx_config.data.io_addr;
    data_passing->num_tx_clients = net_tx_config.num_clients;
    for (int i = 0; i < net_tx_config.num_clients; i++) {
        data_passing->tx_data_paddrs[i] = net_tx_config.clients[i].data.io_addr;
    }

    /* virtio setup: the only virtio device is the console device */
    int console_vdev_idx = 0;
    assert(vmm_config.num_virtio_mmio_devices == 1);
    assert(serial_config.rx.data.vaddr);

    serial_queue_init(&serial_rx_queue, serial_config.rx.queue.vaddr, serial_config.rx.data.size,
                      serial_config.rx.data.vaddr);
    serial_queue_init(&serial_tx_queue, serial_config.tx.queue.vaddr, serial_config.tx.data.size,
                      serial_config.tx.data.vaddr);

    /* Initialise virtIO console device */
    success = virtio_mmio_console_init(&virtio_console, vmm_config.virtio_mmio_devices[console_vdev_idx].base,
                                       vmm_config.virtio_mmio_devices[console_vdev_idx].size,
                                       vmm_config.virtio_mmio_devices[console_vdev_idx].irq, &serial_rx_queue,
                                       &serial_tx_queue, serial_config.tx.id);
    assert(success);

    /* Finally start the guest */
    guest_start(kernel_pc, vmm_config.dtb, vmm_config.initrd);
    LOG_VMM("%s is ready\n", microkit_name);
}

void notified(microkit_channel ch)
{
    if (ch == serial_config.rx.id) {
        virtio_console_handle_rx(&virtio_console);
    } else if (ch == serial_config.tx.id) {
        // Nothing to do
    } else if (ch == net_drv_config.virt_tx.id) {
        if (!virq_inject(uio_tx_irq_num)) {
            LOG_VMM_ERR("failed to inject TX UIO IRQ\n");
        }
    } else if (ch == net_drv_config.virt_rx.id) {
        if (!virq_inject(uio_rx_irq_num)) {
            LOG_VMM_ERR("failed to inject RX UIO IRQ\n");
        }
    } else if (!virq_handle_passthrough(ch)) {
        LOG_VMM_ERR("Unexpected channel, ch: 0x%lx\n", ch);
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
