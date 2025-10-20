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

#if defined(CONFIG_ARCH_AARCH64)
#include <libvmm/arch/aarch64/linux.h>
#include <libvmm/arch/aarch64/fault.h>
#elif defined(CONFIG_ARCH_X86_64)
#include <libvmm/arch/x86_64/linux.h>
#include <libvmm/arch/x86_64/fault.h>
#endif

/*
 * As this is just an example, for simplicity we just make the size of the
 * guest's "RAM" the same for all platforms. For just booting Linux with a
 * simple user-space, 0x10000000 bytes (256MB) is plenty.
 */
#define GUEST_RAM_SIZE 0x10000000

#if defined(BOARD_qemu_virt_aarch64)
#define GUEST_DTB_VADDR 0x4f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x4d000000
#elif defined(BOARD_odroidc4)
#define GUEST_DTB_VADDR 0x2f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x2d700000
#elif defined(BOARD_maaxboard)
#define GUEST_DTB_VADDR 0x4f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x4c000000
#elif defined(BOARD_x86_64_generic_vtx)
#define GUEST_CMDLINE "earlycon=uart8250,io,0x3f8,115200n8 console=ttyS0,115200n8 loglevel=8 keep_bootcon"
#else
#error Need to define guest kernel image address and DTB address on ARM or command line arguments on x86
#endif

/* For simplicity we just enforce the serial IRQ channel number to be the same
 * across platforms. */
#define SERIAL_IRQ_CH 1

#if defined(BOARD_qemu_virt_aarch64)
#define SERIAL_IRQ 33
#elif defined(BOARD_odroidc4)
#define SERIAL_IRQ 225
#elif defined(BOARD_maaxboard)
#define SERIAL_IRQ 58
#elif defined(BOARD_x86_64_generic_vtx)
// @billn revisit
#define SERIAL_IRQ 0
#else
#error Need to define serial interrupt
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

static void serial_ack(size_t vcpu_id, int irq, void *cookie)
{
    /*
     * For now we by default simply ack the serial IRQ, we have not
     * come across a case yet where more than this needs to be done.
     */
    microkit_irq_ack(SERIAL_IRQ_CH);
}

void init(void)
{
    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", microkit_name);
    /* Place all the binaries in the right locations before starting the guest */
    size_t kernel_size = _guest_kernel_image_end - _guest_kernel_image;
    size_t dtb_size = _guest_dtb_image_end - _guest_dtb_image;
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;

#if defined(CONFIG_ARCH_AARCH64)
    uintptr_t kernel_pc = linux_setup_images(guest_ram_vaddr, (uintptr_t)_guest_kernel_image, kernel_size,
                                             (uintptr_t)_guest_dtb_image, GUEST_DTB_VADDR, dtb_size,
                                             (uintptr_t)_guest_initrd_image, GUEST_INIT_RAM_DISK_VADDR, initrd_size);

    if (!kernel_pc) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }
#elif defined(CONFIG_ARCH_X86_64)
    linux_x86_setup_ret_t linux_setup;
    if (!linux_setup_images(guest_ram_vaddr, 0x10000000, (uintptr_t)_guest_kernel_image, kernel_size, 0, 0,
                            GUEST_CMDLINE, &linux_setup)) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }
#else
#error unsupported architecture
#endif

    /* Initialise the virtual GIC driver */
    bool success = virq_controller_init();
    if (!success) {
        LOG_VMM_ERR("Failed to initialise emulated interrupt controller\n");
        return;
    }

    success = virq_register(GUEST_BOOT_VCPU_ID, SERIAL_IRQ, &serial_ack, NULL);
    /* Just in case there is already an interrupt available to handle, we ack it here. */
    // @billn revisit
    // microkit_irq_ack(SERIAL_IRQ_CH);

#ifdef CONFIG_ARCH_X86_64
    // @billn revisit
    seL4_X86_VCPU_EnableIOPort(BASE_VCPU_CAP + GUEST_BOOT_VCPU_ID, BASE_IOPORT_CAP + 10, 0x3f8, 0x3f8 + 7);

    /* Finally start the guest */
    guest_start(linux_setup.kernel_entry_gpa, 0, 0, &linux_setup);
#else
    guest_start(kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR, null);
#endif
}

void notified(microkit_channel ch)
{
    switch (ch) {
    case SERIAL_IRQ_CH: {
        bool success = virq_inject(SERIAL_IRQ);
        if (!success) {
            LOG_VMM_ERR("IRQ %d dropped\n", SERIAL_IRQ);
        }
        break;
    }
    default:
        printf("Unexpected channel, ch: 0x%lx\n", ch);
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
