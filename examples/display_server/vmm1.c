/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <sel4cp.h>
#include "util/util.h"
#include "arch/aarch64/vgic/vgic.h"
#include "arch/aarch64/linux.h"
#include "arch/aarch64/fault.h"
#include "guest.h"
#include "virq.h"
#include "tcb.h"
#include "vcpu.h"

#if defined(BOARD_qemu_arm_virt)
#define GUEST_RAM_SIZE 0x20000000
#define GUEST_DTB_VADDR 0x4f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x5d700000
#elif defined(BOARD_odroidc4)
#define GUEST_RAM_SIZE 0x10000000
#define GUEST_DTB_VADDR 0x2f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x2d700000
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
/* seL4CP will set this variable to the start of the guest RAM memory region. */
uintptr_t guest_ram_vaddr;

#define MAX_IRQ_CH 63
int passthrough_irq_map[MAX_IRQ_CH];

static void passthrough_device_ack(size_t vcpu_id, int irq, void *cookie) {
    sel4cp_channel irq_ch = (sel4cp_channel)(int64_t)cookie;
    sel4cp_irq_ack(irq_ch);
}

static void register_passthrough_irq(int irq, sel4cp_channel irq_ch) {
    LOG_VMM("Register passthrough IRQ %d (channel: 0x%lx)\n", irq, irq_ch);
    assert(irq_ch < MAX_IRQ_CH);
    passthrough_irq_map[irq_ch] = irq;

    int err = virq_register(GUEST_VCPU_ID, irq, &passthrough_device_ack, (void *)(int64_t)irq_ch);
    if (!err) {
        LOG_VMM_ERR("Failed to register IRQ %d\n", irq);
        return;
    }
}

void init(void) {
    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", sel4cp_name);
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

    // serial
    register_passthrough_irq(33, 1);
    // Net
    register_passthrough_irq(79, 2);
    // GPU
    register_passthrough_irq(36, 3);
    // keyboard
    register_passthrough_irq(37, 4);
    // mouse
    register_passthrough_irq(38, 5);

    /* Finally start the guest */
    guest_start(GUEST_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
}

void notified(sel4cp_channel ch) {
    switch (ch) {
        default:
            if (passthrough_irq_map[ch]) {
                bool success = virq_inject(GUEST_VCPU_ID, passthrough_irq_map[ch]);
                if (!success) {
                    LOG_VMM_ERR("IRQ %d dropped on vCPU %d\n", passthrough_irq_map[ch], GUEST_VCPU_ID);
                }
                break;
            }
            printf("Unexpected channel, ch: 0x%lx\n", ch);
    }
}

/*
 * The primary purpose of the VMM after initialisation is to act as a fault-handler,
 * whenever our guest causes an exception, it gets delivered to this entry point for
 * the VMM to handle.
 */
void fault(sel4cp_id id, sel4cp_msginfo msginfo) {
    bool success = fault_handle(id, msginfo);
    if (success) {
        /* Now that we have handled the fault successfully, we reply to it so
         * that the guest can resume execution. */
        sel4cp_fault_reply(sel4cp_msginfo_new(0, 0));
    }
}
