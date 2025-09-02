/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <libvmm/guest.h>
#include <libvmm/virq.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/aarch64/linux.h>
#include <libvmm/arch/aarch64/fault.h>

/*
 * As this is just an example, for simplicity we just make the size of the
 * guest's "RAM" the same for all platforms. For just booting Linux with a
 * simple user-space, 0x10000000 bytes (256MB) is plenty.
 */
#define GUEST_RAM_SIZE 0x2b000000
#define PAGE_SIZE_4K 0x1000

#if defined(BOARD_qemu_virt_aarch64)
#define GUEST_DTB_VADDR 0x4f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x4d700000
#elif defined(BOARD_rpi4b_1gb)
#define GUEST_DTB_VADDR 0x1e000000
#define GUEST_INIT_RAM_DISK_VADDR 0x1c000000
#elif defined(BOARD_odroidc2_hyp)
#define GUEST_DTB_VADDR 0x2f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x2d700000
#elif defined(BOARD_odroidc4)
#define GUEST_DTB_VADDR 0x2f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x2d700000
#elif defined(BOARD_maaxboard)
#define GUEST_DTB_VADDR 0x4f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x4c000000
#else
#error Need to define guest kernel image address and DTB address
#endif

/* For simplicity we just enforce the serial IRQ channel number to be the same
 * across platforms. */
#define SERIAL_IRQ_CH                           1
#define MMC_IRQ_CH                              18

#define CAMERA_CLIENT_CH                        4
#define CLIENT_NOTIFY_VMM_IRQ                   160                      
#define GUEST_TO_VMM_FRAME_READY_FAULT_ADDR     0xd0000000

#if defined(BOARD_qemu_virt_aarch64)
#define SERIAL_IRQ 33
#elif defined(BOARD_odroidc2_hyp) || defined(BOARD_odroidc4)
#define SERIAL_IRQ 225
#elif defined(BOARD_rpi4b_1gb)
#define SERIAL_IRQ 125
#define MMC_IRQ 158
#elif defined(BOARD_imx8mm_evk)
#define SERIAL_IRQ 59
#elif defined(BOARD_imx8mq_evk) || defined(BOARD_maaxboard)
#define SERIAL_IRQ 58
#else
#error Need to define serial interrupt
#endif

struct microkit_irq {
    int irq;
    microkit_channel ch;
};

struct microkit_irq irqs[] = {
    {
        .irq = SERIAL_IRQ,
        .ch = SERIAL_IRQ_CH
    },
    // ethernet
    {
        .irq = 189,
        .ch = 2
    },
    {
        .irq = 190,
        .ch = 3
    },
    // mailbox
    {
        .irq = 65,
        .ch = 29
    },
    {
        .irq = 66,
        .ch = 33
    },
    {
        /* MMC1 */
        .irq = MMC_IRQ,
        .ch = MMC_IRQ_CH
    },
    {
        .irq = 152,
        .ch = 30
    },
    // other interrupt controller
    {
        .irq = 128,
        .ch = 28
    }, 
    // CSI interrupts
    {
        .irq = 135,
        .ch = 16
    },
    {
        .irq = 134,
        .ch = 17
    },
    // i2c
    {
        .irq = 149,
        .ch = 19
    },
    // txp
    {
        .irq = 107,
        .ch = 20
    },
    // hvs
    {
        .irq = 129,
        .ch = 21
    },
    // pixel valves
    {
        .irq = 142,
        .ch = 22
    },
    {
        .irq = 138,
        .ch = 23
    },
    {
        .irq = 133,
        .ch = 24
    },
    {
        .irq = 141,
        .ch = 25
    },
    // GPIO
    {
        .irq = 145,
        .ch = 31
    },
    {
        .irq = 146,
        .ch = 32
    },
    {
        .irq = 208,
        .ch = 35
    },
    // dma
    {
        .irq = 121,
        .ch = 36
    },
    {
        .irq = 122,
        .ch = 37
    },
    {
        .irq = 123,
        .ch = 38
    },
    {
        .irq = 124,
        .ch = 39
    },
    {
        .irq = 5,
        .ch = 40
    },
    {
        .irq = 112,
        .ch = 41
    },
    {
        .irq = 113,
        .ch = 42
    },
    {
        .irq = 114,
        .ch = 43
    },
    {
        .irq = 115,
        .ch = 44
    },
    {
        .irq = 116,
        .ch = 45
    },
    {
        .irq = 117,
        .ch = 46
    },
    {
        .irq = 118,
        .ch = 47
    },
    {
        .irq = 119,
        .ch = 48
    },
    {
        .irq = 120,
        .ch = 49
    },
    {
        .irq = 156,
        .ch = 50
    },
};

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

static void serial_ack(size_t vcpu_id, int irq, void *cookie) {
    /*
     * For now we by default simply ack the serial IRQ, we have not
     * come across a case yet where more than this needs to be done.
     */
    microkit_irq_ack(SERIAL_IRQ_CH);
}

static void uio_ack(size_t vcpu_id, int irq, void *cookie) {
}

// signal client when frame buffer is full 
bool uio_camera_vmm_br_signal(size_t vcpu_id, uintptr_t addr, size_t fsr, seL4_UserContext *regs, void *data)
{
    LOG_VMM("Notifying Client\n");

    microkit_notify(CAMERA_CLIENT_CH);
    LOG_VMM("Returned from Notifying Client\n");

    return true;
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
    for (int i = 0; i < sizeof(irqs) / sizeof(struct microkit_irq); i++) {
        success = virq_register_passthrough(GUEST_VCPU_ID, irqs[i].irq, irqs[i].ch);
        if (!success) {
            LOG_VMM_ERR("Failed to register passthrough IRQ %d\n", irqs[i].irq);
            return;
        }
    }

    success = virq_register(GUEST_VCPU_ID, CLIENT_NOTIFY_VMM_IRQ, uio_ack, NULL);
    if (!success) {
        LOG_VMM_ERR("Failed to register IRQ %d\n", CLIENT_NOTIFY_VMM_IRQ);
        return;
    }

    bool frame_ready_vmfault_reg_ok = fault_register_vm_exception_handler(GUEST_TO_VMM_FRAME_READY_FAULT_ADDR, PAGE_SIZE_4K,
                                                                 &uio_camera_vmm_br_signal, NULL);
    if (!frame_ready_vmfault_reg_ok) {
        LOG_VMM_ERR("Failed to register the VM fault handler for frame transmit\n");
        return;
    }

    /* Finally start the guest */
    guest_start(GUEST_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
}

void notified(microkit_channel ch) {
    bool handled = virq_handle_passthrough(ch);
    switch (ch) {
    case CAMERA_CLIENT_CH:
        if (!virq_inject(GUEST_VCPU_ID, CLIENT_NOTIFY_VMM_IRQ)) {
            LOG_VMM_ERR("failed to inject Control Region Update UIO IRQ\n");
        }
        break;
    default:
        if (handled) {
            return;
        }
        printf("Unexpected channel, ch: 0x%lx\n", ch);
    }
}

/*
 * The primary purpose of the VMM after initialisation is to act as a fault-handler.
 * Whenever our guest causes an exception, it gets delivered to this entry point for
 * the VMM to handle.
 */
seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo) {
    bool success = fault_handle(child, msginfo);
    if (success) {
        /* Now that we have handled the fault successfully, we reply to it so
         * that the guest can resume execution. */
        *reply_msginfo = microkit_msginfo_new(0, 0);
        return seL4_True;
    }

    return seL4_False;
}
