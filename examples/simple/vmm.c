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

/*
 * As this is just an example, for simplicity we just make the size of the
 * guest's "RAM" the same for all platforms. For just booting Linux with a
 * simple user-space, 0x10000000 bytes (256MB) is plenty.
 */
#define GUEST_RAM_SIZE 0x10000000

/* For ARM, these constants depends on what's defined in your DTB. */
#if defined(BOARD_qemu_virt_aarch64)
#define GUEST_RAM_START_GPA 0x40000000
#define GUEST_DTB_GPA 0x4f000000
#define GUEST_INIT_RAM_DISK_GPA 0x4d000000
#elif defined(BOARD_odroidc4)
#define GUEST_RAM_START_GPA 0x20000000
#define GUEST_DTB_GPA 0x2f000000
#define GUEST_INIT_RAM_DISK_GPA 0x2d700000
#elif defined(BOARD_maaxboard)
#define GUEST_RAM_START_GPA 0x40000000
#define GUEST_DTB_GPA 0x4f000000
#define GUEST_INIT_RAM_DISK_GPA 0x4c000000
#elif defined(BOARD_x86_64_generic_vtx)
#define TIMER_DRV_CH 10
#define GUEST_RAM_START_GPA LOW_RAM_START_GPA
#define GUEST_CMDLINE "earlyprintk=serial,0x3f8,115200 debug console=ttyS0,115200 earlycon=serial,0x3f8,115200 loglevel=8"
#else
#error Need to define guest kernel image address and DTB address
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
#define COM1_IOAPIC_CHIP 0
#define COM1_IOAPIC_PIN 4
#define COM1_IO_PORT_ID 0
#define COM1_IO_PORT_ADDR 0x3F8
#define COM1_IO_PORT_SIZE 8
#else
#error Need to define serial interrupt
#endif

/* Data for the guest's kernel image. */
extern char _guest_kernel_image[];
extern char _guest_kernel_image_end[];
/* Data for the initial RAM disk to be passed to the kernel. */
extern char _guest_initrd_image[];
extern char _guest_initrd_image_end[];

#if defined(CONFIG_ARCH_X86_64)
/* Data for the guest's ACPI Differentiated System Description Table (DSDT). */
extern char _guest_dsdt_aml[];
extern char _guest_dsdt_aml_end[];
#else
/* Data for the device tree to be passed to the kernel. */
extern char _guest_dtb_image[];
extern char _guest_dtb_image_end[];
#endif

/* Microkit will set this variable to the start of the guest RAM memory region. */
uintptr_t guest_ram_vaddr;

void init(void)
{
    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", microkit_name);

    arch_guest_init_t args = {
#if defined(CONFIG_ARCH_X86_64)
        .bsp = true,
        .timer_ch = TIMER_DRV_CH,
#elif defined(CONFIG_ARCH_ARM)
        .num_vcpus = 1,
#endif
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
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;

    if (!kernel_size) {
        LOG_VMM_ERR("Kernel image is empty\n");
        return;
    }
    if (!initrd_size) {
        LOG_VMM_ERR("Initial ramdisk image is empty\n");
        return;
    }

#if defined(CONFIG_ARCH_X86_64)
    size_t dsdt_aml_size = _guest_dsdt_aml_end - _guest_dsdt_aml;

    if (!dsdt_aml_size) {
        LOG_VMM_ERR("DSDT AML image is empty\n");
        return;
    }

    seL4_VCPUContext initial_regs;
    linux_x86_setup_ret_t linux_setup;
    if (!linux_setup_images((uintptr_t)_guest_kernel_image, kernel_size, (uintptr_t)_guest_initrd_image, initrd_size,
                            _guest_dsdt_aml, dsdt_aml_size, GUEST_CMDLINE, &initial_regs, &linux_setup)) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }

    /* Pass through COM1 serial port */
    microkit_vcpu_x86_enable_ioport(GUEST_BOOT_VCPU_ID, COM1_IO_PORT_ID, COM1_IO_PORT_ADDR, COM1_IO_PORT_SIZE);
    microkit_irq_ack(SERIAL_IRQ_CH);

    /* Pass through serial IRQs */
    assert(virq_ioapic_register_passthrough(COM1_IOAPIC_CHIP, COM1_IOAPIC_PIN, SERIAL_IRQ_CH));

    guest_start_long_mode(linux_setup.kernel_entry_gpa, linux_setup.pml4_gpa, linux_setup.gdt_gpa,
                          linux_setup.gdt_limit, &initial_regs);
#else
    size_t dtb_size = _guest_dtb_image_end - _guest_dtb_image;
    if (!dtb_size) {
        LOG_VMM_ERR("DTB image is empty\n");
        return;
    }

    uintptr_t kernel_pc = linux_setup_images(GUEST_RAM_START_GPA, (uintptr_t)_guest_kernel_image, kernel_size,
                                             (uintptr_t)_guest_dtb_image, GUEST_DTB_GPA, dtb_size,
                                             (uintptr_t)_guest_initrd_image, GUEST_INIT_RAM_DISK_GPA, initrd_size);
    if (!kernel_pc) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }

    success = virq_register_passthrough(GUEST_BOOT_VCPU_ID, SERIAL_IRQ, SERIAL_IRQ_CH);
    assert(success);
    /* Finally start the guest */
    guest_start(kernel_pc, GUEST_DTB_GPA, GUEST_INIT_RAM_DISK_GPA);
#endif
}

void notified(microkit_channel ch)
{
#if defined(CONFIG_ARCH_X86_64)
    /* In our case a VCPU is always running, so if we get notified then it must've mean that a
     * VCPU was stopped, we need to save the context of the VCPU so that we can resume it again
     * later, since the IPC buffer maybe clobbered. */
    vcpu_init_exit_state(true);
#endif

    switch (ch) {
#if defined(CONFIG_ARCH_X86_64)
    case TIMER_DRV_CH: {
        guest_time_handle_timer_ntfn();
        break;
    }
    case SERIAL_IRQ_CH: {
        bool success = virq_ioapic_handle_passthrough(ch);
        if (!success) {
            LOG_VMM_ERR("I/O APIC IRQ pin %d dropped\n", COM1_IOAPIC_PIN);
        }
        break;
    }
#else
    case SERIAL_IRQ_CH: {
        bool success = virq_handle_passthrough(ch);
        if (!success) {
            LOG_VMM_ERR("IRQ %d dropped\n", SERIAL_IRQ);
        }
        break;
    }
    default:
        printf("Unexpected channel, ch: 0x%x\n", ch);
#endif
    }

#if defined(CONFIG_ARCH_X86_64)
    vcpu_exit_resume();
#endif
}

/*
 * The primary purpose of the VMM after initialisation is to act as a fault-handler.
 * Whenever our guest causes an exception, it gets delivered to this entry point for
 * the VMM to handle.
 */
seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo)
{
#if defined(CONFIG_ARCH_X86_64)
    fault_handle(child);
    return seL4_True;
#else
    bool success = fault_handle(child, msginfo);
    if (success) {
        /* Now that we have handled the fault successfully, we reply to it so
         * that the guest can resume execution. */
        *reply_msginfo = microkit_msginfo_new(0, 0);
        return seL4_True;
    }

    return seL4_False;
#endif
}
