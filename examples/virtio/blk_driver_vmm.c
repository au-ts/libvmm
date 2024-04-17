/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <microkit.h>
#include <util.h>
#include <vgic/vgic.h>
#include <linux.h>
#include <fault.h>
#include <guest.h>
#include <virq.h>
#include <tcb.h>
#include <vcpu.h>

// #define DEBUG_BLK_DRIVER_VM

#define GUEST_RAM_SIZE 0x6000000

#if defined(BOARD_qemu_arm_virt)
#define GUEST_DTB_VADDR 0x47f00000
#define GUEST_INIT_RAM_DISK_VADDR 0x47000000
#elif defined(BOARD_odroidc4)
#define GUEST_DTB_VADDR 0x25f10000
#define GUEST_INIT_RAM_DISK_VADDR 0x24000000
#else
#error Need to define guest kernel image address and DTB address
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

/* sDDF block */
#if defined(BOARD_odroidc4)
#define SD_IRQ 222
#define SD_CH 1 // @ericc: autogen
#endif

// @ericc: autogen
#define UIO_IRQ 50
#define UIO_CH 3

void uio_ack(size_t vcpu_id, int irq, void *cookie) {
    microkit_notify(UIO_CH);
}

void init(void) {
    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", microkit_name);
    /* Place all the binaries in the right locations before starting the guest */
    size_t kernel_size = _guest_kernel_image_end - _guest_kernel_image;
    size_t dtb_size = _guest_dtb_image_end - _guest_dtb_image;
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;
    uintptr_t kernel_pc = linux_setup_images(guest_ram_vaddr,
                                      (uintptr_t) _guest_kernel_image,
                                      kernel_size,
                                      (uintptr_t) _guest_dtb_image,
                                      GUEST_DTB_VADDR,
                                      dtb_size,
                                      (uintptr_t) _guest_initrd_image,
                                      GUEST_INIT_RAM_DISK_VADDR,
                                      initrd_size
                                      );
    if (!kernel_pc) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }
    
    /* Initialise the virtual GIC driver */
    bool success = virq_controller_init(GUEST_VCPU_ID);
    if (!success) {
        LOG_VMM_ERR("Failed to initialise emulated interrupt controller\n");
        return;
    }

    /* Register the UIO IRQ */
    virq_register(GUEST_VCPU_ID, UIO_IRQ, uio_ack, NULL);

#if defined(BOARD_odroidc4)
    /* Register the SD card IRQ */
    virq_register_passthrough(GUEST_VCPU_ID, SD_IRQ, SD_CH);
#if defined(DEBUG_BLK_DRIVER_VM)
    virq_register_passthrough(GUEST_VCPU_ID, 225, 10);
#endif
#endif

#if defined(BOARD_qemu_arm_virt)
#if defined(DEBUG_BLK_DRIVER_VM)
    virq_register_passthrough(GUEST_VCPU_ID, 33, 1);
#endif
#endif

    /* Finally start the guest */
    guest_start(GUEST_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
}

void notified(microkit_channel ch) {
    bool handled = false;

    handled = virq_handle_passthrough(ch);

    if (ch == UIO_CH) {
        virq_inject(GUEST_VCPU_ID, UIO_IRQ);
        handled = true;
    }

    if (!handled) {
        LOG_VMM_ERR("Unhandled notification on channel %d\n", ch);
    }
}

/*
 * The primary purpose of the VMM after initialisation is to act as a fault-handler,
 * whenever our guest causes an exception, it gets delivered to this entry point for
 * the VMM to handle.
 */
void fault(microkit_id id, microkit_msginfo msginfo) {
    bool success = fault_handle(id, msginfo);
    if (success) {
        /* Now that we have handled the fault successfully, we reply to it so
         * that the guest can resume execution. */
        microkit_fault_reply(microkit_msginfo_new(0, 0));
    }
}
