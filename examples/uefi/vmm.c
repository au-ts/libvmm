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

#define TIMER_DRV_CH 10
#define GUEST_RAM_START_GPA LOW_RAM_START_GPA
#define GUEST_FLASH_START_GPA 0xffa00000 /* Correspond to SDF */
#define GUEST_CMDLINE "earlyprintk=serial,0x3f8,115200 debug console=ttyS0,115200 earlycon=serial,0x3f8,115200 loglevel=8"

#define SERIAL_IRQ_CH 1

#define COM1_IOAPIC_CHIP 0
#define COM1_IOAPIC_PIN 4
#define COM1_IO_PORT_ID 0
#define COM1_IO_PORT_ADDR 0x3F8
#define COM1_IO_PORT_SIZE 8

/* Data for the guest's kernel image. */
extern char _guest_kernel_image[];
extern char _guest_kernel_image_end[];
/* Data for the initial RAM disk to be passed to the kernel. */
extern char _guest_initrd_image[];
extern char _guest_initrd_image_end[];
/* Data for the guest's ACPI Differentiated System Description Table (DSDT). */
extern char _guest_dsdt_aml[];
extern char _guest_dsdt_aml_end[];
/* Data for the guest's UEFI firmware. */
extern char _guest_firmware[];
extern char _guest_firmware_end[];

/* Microkit will set this variable to the start of the guest RAM memory and flash regions. */
uintptr_t guest_ram_vaddr;
uint64_t guest_ram_size;
uintptr_t guest_flash_vaddr;
uint64_t guest_flash_size;

/* OVMF expects a standard PC PCI bus, so we just make a small aperature at an arbitrary guest
 * RAM location to trigger libvmm to build a virtual PCI bus. But otherwise this example doesn't
 * use the virtual PCI unless you extend it to do so. */
#define PCI_MMIO_APERATURE_GPA 0xE0000000
#define PCI_MMIO_APERATURE_SIZE 0x200000

void init(void)
{
    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", microkit_name);

    arch_guest_init_t args = { .pci_init.mmio_aperature_gpa = PCI_MMIO_APERATURE_GPA,
                               .pci_init.mmio_aperature_size = PCI_MMIO_APERATURE_SIZE,
                               .bsp = true,
                               .timer_ch = TIMER_DRV_CH,
                               .num_guest_ram_regions = 2,
                               .guest_ram_regions = {
                                   (struct guest_ram_region) { .gpa_start = GUEST_RAM_START_GPA,
                                                               .size = guest_ram_size,
                                                               .vmm_vaddr = (void *)guest_ram_vaddr },
                                   (struct guest_ram_region) { .gpa_start = GUEST_FLASH_START_GPA,
                                                               .size = guest_flash_size,
                                                               .vmm_vaddr = (void *)guest_flash_vaddr } } };
    bool success = guest_init(args);
    if (!success) {
        LOG_VMM_ERR("Failed to initialise guest\n");
        return;
    }

    /* Place all the binaries in the right locations before starting the guest */
    size_t kernel_size = _guest_kernel_image_end - _guest_kernel_image;
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;
    size_t dsdt_aml_size = _guest_dsdt_aml_end - _guest_dsdt_aml;
    size_t firmware_size = _guest_firmware_end - _guest_firmware;

    if (!kernel_size) {
        LOG_VMM_ERR("Kernel image is empty\n");
        return;
    }
    if (!initrd_size) {
        LOG_VMM_ERR("Initial ramdisk image is empty\n");
        return;
    }
    if (!dsdt_aml_size) {
        LOG_VMM_ERR("DSDT AML image is empty\n");
        return;
    }
    if (!firmware_size) {
        LOG_VMM_ERR("Firmware image is empty\n");
        return;
    }

    if (!uefi_setup_images((uintptr_t)_guest_firmware, firmware_size, (uintptr_t)_guest_dsdt_aml, dsdt_aml_size,
                           GUEST_FLASH_START_GPA, guest_flash_size)) {
        LOG_VMM_ERR("Failed to initialise UEFI firmware\n");
        return;
    }

    if (!uefi_add_linux_boot((uintptr_t)_guest_kernel_image, kernel_size, (uintptr_t)_guest_initrd_image, initrd_size,
                             (char *)GUEST_CMDLINE)) {
        LOG_VMM_ERR("Failed to initialise UEFI firmware\n");
        return;
    }

    /* Pass through COM1 serial port */
    microkit_vcpu_x86_enable_ioport(GUEST_BOOT_VCPU_ID, COM1_IO_PORT_ID, COM1_IO_PORT_ADDR, COM1_IO_PORT_SIZE);
    microkit_irq_ack(SERIAL_IRQ_CH);

    /* Pass through serial IRQs */
    assert(virq_register_passthrough(X86_IOAPIC_IRQ_ROUTE(COM1_IOAPIC_CHIP, COM1_IOAPIC_PIN), SERIAL_IRQ_CH));

    guest_start_reset_state();
}

void notified(microkit_channel ch)
{
    switch (ch) {
    case SERIAL_IRQ_CH: {
        bool success = virq_handle_passthrough(ch);
        if (!success) {
            LOG_VMM_ERR("Serial IRQ dropped\n");
        }
        break;
    }
    default:
        printf("Unexpected channel, ch: 0x%x\n", ch);
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
