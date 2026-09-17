/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <libvmm/libvmm.h>

#define GUEST_RAM_SIZE 0x40000000

/* For ARM, these constants depends on what's defined in your DTB. */
#if defined(BOARD_rpi4b_4gb)
#define GUEST_RAM_START_GPA 0x10000000
#define GUEST_DTB_GPA 0x4f000000
#define GUEST_INIT_RAM_DISK_GPA 0x4c000000
#else
#error Need to define guest kernel image address and DTB address
#endif

/* For simplicity we just enforce the serial IRQ channel number to be the same
 * across platforms. */
#define SERIAL_IRQ_CH 1

#if defined(BOARD_rpi4b_4gb)
#define SERIAL_IRQ 125 // serial@7e215040
#define MAILBOX_IRQ 65 // mailbox@7e00b880
#define VCHIQ_IRQ 66 // mailbox@7e00b840
#define WIFI_IRQ 158 // mmcnr@7e300000
#define GPIO_IRQ_1 145 // gpio@7e200000
#define GPIO_IRQ_2 146 // gpio@7e200000
#define ETH_IRQ_1 189 // ethernet@7d580000
#define ETH_IRQ_2 190 // ethernet@7d580000

#define MAILBOX_IRQ_CH 2
#define VCHIQ_IRQ_CH 3
#define WIFI_IRQ_CH 4
#define GPIO_IRQ_1_CH 5
#define GPIO_IRQ_2_CH 6
#define ETH_IRQ_1_CH 7
#define ETH_IRQ_2_CH 8
#else
#error Need to define interrupts
#endif

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

void init(void)
{
    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", microkit_name);

    arch_guest_init_t args = {
        .num_vcpus = 1,
        .num_guest_ram_regions = 1,
        .guest_ram_regions = { (struct guest_ram_region) {
            .gpa_start = GUEST_RAM_START_GPA, .size = GUEST_RAM_SIZE, .vmm_vaddr = (void *)guest_ram_vaddr } }
    };
    bool success = guest_init(args);
    if (!success) {
        LOG_VMM_ERR("Failed to initialise guest\n");
        return;
    }

    /* Place all the binaries in the right locations before starting the guest */
    size_t kernel_size = _guest_kernel_image_end - _guest_kernel_image;
    size_t dtb_size = _guest_dtb_image_end - _guest_dtb_image;
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;
    uintptr_t kernel_pc = linux_setup_images(GUEST_RAM_START_GPA, (uintptr_t)_guest_kernel_image, kernel_size,
                                             (uintptr_t)_guest_dtb_image, GUEST_DTB_GPA, dtb_size,
                                             (uintptr_t)_guest_initrd_image, GUEST_INIT_RAM_DISK_GPA, initrd_size);
    if (!kernel_pc) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }

    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, SERIAL_IRQ, SERIAL_IRQ_CH));

    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, MAILBOX_IRQ, MAILBOX_IRQ_CH));
    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, VCHIQ_IRQ, VCHIQ_IRQ_CH));
    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, WIFI_IRQ, WIFI_IRQ_CH));
    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, GPIO_IRQ_1, GPIO_IRQ_1_CH));
    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, GPIO_IRQ_2, GPIO_IRQ_2_CH));
    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, ETH_IRQ_1, ETH_IRQ_1_CH));
    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, ETH_IRQ_2, ETH_IRQ_2_CH));

    // DMA channel 0-10
    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, 112, 9));
    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, 113, 10));
    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, 114, 11));
    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, 115, 12));
    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, 116, 13));
    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, 117, 14));
    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, 118, 15));
    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, 119, 16));
    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, 120, 17));
    assert(virq_register_passthrough(GUEST_BOOT_VCPU_ID, 121, 18));

    /* Finally start the guest */
    guest_start(kernel_pc, GUEST_DTB_GPA, GUEST_INIT_RAM_DISK_GPA);
}

void notified(microkit_channel ch)
{
    if (!virq_handle_passthrough(ch)) {
        LOG_VMM_ERR("Failed to handle IRQ passthrough channel %u\n", ch);
    }
}

/*
 * The primary purpose of the VMM after initialisation is to act as a fault-handler.
 * Whenever our guest causes an exception, it gets delivered to this entry point for
 * the VMM to handle.
 */
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
