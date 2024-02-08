/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <microkit.h>
#include "util/util.h"
#include "arch/aarch64/vgic/vgic.h"
#include "arch/aarch64/linux.h"
#include "arch/aarch64/fault.h"
#include "guest.h"
#include "virq.h"
#include "tcb.h"
#include "vcpu.h"

// @ivanv: ideally we would have none of these hardcoded values
// initrd, ram size come from the DTB
// We can probably add a node for the DTB addr and then use that.
// Part of the problem is that we might need multiple DTBs for the same example
// e.g one DTB for VMM one, one DTB for VMM two. we should be able to hide all
// of this in the build system to avoid doing any run-time DTB stuff.

/*
 * As this is just an example, for simplicity we just make the size of the
 * guest's "RAM" the same for all platforms. For just booting Linux with a
 * simple user-space, 0x10000000 bytes (256MB) is plenty.
 */
#define GUEST_RAM_SIZE 0x10000000

#if defined(BOARD_qemu_arm_virt)
#define GUEST_DTB_VADDR 0x4f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x4d700000
#elif defined(BOARD_rpi4b_hyp)
#define GUEST_DTB_VADDR 0x2e000000
#define GUEST_INIT_RAM_DISK_VADDR 0x2d700000
#elif defined(BOARD_odroidc2_hyp)
#define GUEST_DTB_VADDR 0x2f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x2d700000
#elif defined(BOARD_odroidc4)
#define GUEST_DTB_VADDR 0x2f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x2d700000
#elif defined(BOARD_imx8mm_evk_hyp)
#define GUEST_DTB_VADDR 0x4f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x4d700000
#else
#error Need to define guest kernel image address and DTB address
#endif

#if defined(BOARD_qemu_arm_virt)
#define PASSTHROUGH_IRQ_COUNT 2
static int passthrough_irqs[PASSTHROUGH_IRQ_COUNT] = {
    33, // serial
    37, // pcie
};
#elif defined(BOARD_odroidc2_hyp) || defined(BOARD_odroidc4)
#define PASSTHROUGH_IRQ_COUNT 5
static int passthrough_irqs[PASSTHROUGH_IRQ_COUNT] = {
    225,        // Serial
    48, 63, 62, // USB (controller, 1, 2)
    5           // @alexbr: not sure what this is
};
#elif defined(BOARD_rpi4b_hyp)
#define SERIAL_IRQ 57
#elif defined(BOARD_imx8mm_evk_hyp)
#define SERIAL_IRQ 79
#else
#error Need to define passthrough IRQs
#endif

#define MAX_IRQ 128
static int irq_channels[MAX_IRQ] = {0};

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

static void passthrough_ack(size_t vcpu_id, int irq, void *cookie) {
    int *ch = cookie;
    microkit_irq_ack(*ch);
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

    for (int i = 0; i < PASSTHROUGH_IRQ_COUNT; i++) {
        int irq = passthrough_irqs[i];
        int ch = i + 1;
        irq_channels[irq] = ch;

        success = virq_register(GUEST_VCPU_ID, irq, &passthrough_ack, &irq_channels[irq]);
        if (!success) {
            LOG_VMM_ERR("Failed to register IRQ %d\n", i);
            return;
        }
        microkit_irq_ack(ch);
    }

    /* Finally start the guest */
    guest_start(GUEST_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
}

void notified(microkit_channel ch) {

    if (ch > 0 && ch <= PASSTHROUGH_IRQ_COUNT) {
        int irq = passthrough_irqs[ch - 1];

        bool success = virq_inject(GUEST_VCPU_ID, irq);
        if (!success) {
            LOG_VMM_ERR("IRQ %d dropped on vCPU %d\n", irq, GUEST_VCPU_ID);
        }
    } else {
        printf("Unexpected channel, ch: 0x%lx\n", ch);
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
