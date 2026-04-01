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
#include <sddf/timer/client.h>

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

#if defined(BOARD_x86_64_generic_vtx)
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

#if defined(BOARD_x86_64_generic_vtx)
static void serial_ack(int ioapic, int pin, void *cookie)
#else
static void serial_ack(size_t vcpu_id, int irq, void *cookie)
#endif
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
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;

    if (!kernel_size) {
        LOG_VMM_ERR("Kernel image is empty\n");
        return;
    }

    if (!initrd_size) {
        LOG_VMM_ERR("Initial ramdisk image is empty\n");
        return;
    }

#if defined(BOARD_x86_64_generic_vtx)
    linux_x86_setup_ret_t linux_setup;
    size_t dsdt_aml_size = _guest_dsdt_aml_end - _guest_dsdt_aml;

    if (!dsdt_aml_size) {
        LOG_VMM_ERR("DSDT AML image is empty\n");
        return;
    }

    assert(guest_ram_add_region(LOW_RAM_START_GPA, guest_ram_vaddr, GUEST_RAM_SIZE));

    if (!linux_setup_images(guest_ram_vaddr, GUEST_RAM_SIZE, (uintptr_t)_guest_kernel_image, kernel_size,
                            (uintptr_t)_guest_initrd_image, initrd_size, _guest_dsdt_aml, dsdt_aml_size, GUEST_CMDLINE,
                            &linux_setup)) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }

    if (!vcpu_set_up_long_mode(linux_setup.pml4_gpa, linux_setup.gdt_gpa, linux_setup.gdt_limit)) {
        LOG_VMM_ERR("Failed to set up virtual CPU\n");
        return;
    }

    /* Set up the virtual PCI bus */
    assert(pci_x86_init());

    /* Pass through COM1 serial port */
    microkit_vcpu_x86_enable_ioport(GUEST_BOOT_VCPU_ID, COM1_IO_PORT_ID, COM1_IO_PORT_ADDR, COM1_IO_PORT_SIZE);
    microkit_irq_ack(SERIAL_IRQ_CH);

    /* Determine the CPU's TSC frequency */
    uint64_t tsc_hz = get_host_tsc_hz(TIMER_DRV_CH_FOR_LAPIC);
    if (!tsc_hz) {
        LOG_VMM_ERR("cannot determine TSC frequency\n");
        return;
    }

    /* Initialise CPUID */
    if (!initialise_cpuid(tsc_hz)) {
        LOG_VMM_ERR("cannot initialise CPUID\n");
        return;
    }

    /* Initialise the virtual Local and I/O APICs */
    //                                                        @billn revisit vapic vaddr
    bool success = virq_controller_init(tsc_hz, 0xfffffffffffffff);
    if (!success) {
        LOG_VMM_ERR("Failed to initialise virtual IRQ controller\n");
        return;
    }

    /* Pass through serial IRQs */
    assert(virq_ioapic_register(COM1_IOAPIC_CHIP, COM1_IOAPIC_PIN, &serial_ack, NULL));

    guest_start(linux_setup.kernel_entry_gpa, 0, 0);
#else
    size_t dtb_size = _guest_dtb_image_end - _guest_dtb_image;
    if (!dtb_size) {
        LOG_VMM_ERR("DTB image is empty\n");
        return;
    }

    uintptr_t kernel_pc = linux_setup_images(guest_ram_vaddr, (uintptr_t)_guest_kernel_image, kernel_size,
                                             (uintptr_t)_guest_dtb_image, GUEST_DTB_VADDR, dtb_size,
                                             (uintptr_t)_guest_initrd_image, GUEST_INIT_RAM_DISK_VADDR, initrd_size);
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
    success = virq_register(GUEST_BOOT_VCPU_ID, SERIAL_IRQ, &serial_ack, NULL);
    /* Just in case there is already an interrupt available to handle, we ack it here. */
    microkit_irq_ack(SERIAL_IRQ_CH);
    /* Finally start the guest */
    guest_start(kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
#endif
}

void notified(microkit_channel ch)
{
    switch (ch) {
#if defined(BOARD_x86_64_generic_vtx)
    case TIMER_DRV_CH_FOR_LAPIC: {
        handle_lapic_timer_nftn(GUEST_BOOT_VCPU_ID);
        break;
    }
    case TIMER_DRV_CH_FOR_HPET_CH0:
    case TIMER_DRV_CH_FOR_HPET_CH1:
    case TIMER_DRV_CH_FOR_HPET_CH2:
        hpet_handle_timer_ntfn(ch);
        break;
    case SERIAL_IRQ_CH: {
        bool success = inject_ioapic_irq(COM1_IOAPIC_CHIP, COM1_IOAPIC_PIN);
        if (!success) {
            LOG_VMM_ERR("I/O APIC IRQ pin %d dropped\n", COM1_IOAPIC_PIN);
        }
        break;
    }
#else
    case SERIAL_IRQ_CH: {
        bool success = virq_inject(SERIAL_IRQ);
        if (!success) {
            LOG_VMM_ERR("IRQ %d dropped\n", SERIAL_IRQ);
        }
        break;
    }
    default:
        printf("Unexpected channel, ch: 0x%lx\n", ch);
#endif
    }
}

/*
 * The primary purpose of the VMM after initialisation is to act as a fault-handler.
 * Whenever our guest causes an exception, it gets delivered to this entry point for
 * the VMM to handle.
 */
seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo)
{
#if defined(BOARD_x86_64_generic_vtx)
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
