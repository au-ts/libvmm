/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <libvmm/libvmm.h>
#include <sddf/timer/client.h>

uint64_t com1_ioport_id;
uint64_t com1_ioport_addr;
uint64_t com1_ioport_size = 8;

#define GUEST_CMDLINE "earlyprintk=serial,0x3f8,115200 debug console=ttyS0,115200 earlycon=serial,0x3f8,115200 loglevel=8"

/* Data for the guest's kernel image. */
extern char _guest_kernel_image[];
extern char _guest_kernel_image_end[];
/* Data for the initial RAM disk to be passed to the kernel. */
extern char _guest_initrd_image[];
extern char _guest_initrd_image_end[];
/* Data for the guest's ACPI Differentiated System Description Table (DSDT). */
extern char _guest_dsdt_aml[];
extern char _guest_dsdt_aml_end[];

/* Microkit will set this variable to the start of the guest RAM memory region. */
uintptr_t guest_ram_vaddr;
uint64_t guest_ram_size;

uintptr_t guest_ecam_vaddr;
uint64_t guest_ecam_size;

uintptr_t guest_vapic_vaddr;
uint64_t guest_vapic_size;
uint64_t guest_vapic_paddr;

uintptr_t guest_apic_access_vaddr;
uint64_t guest_apic_access_size;
uint64_t guest_apic_access_paddr;

// @billn unused, but have to leave it here otherwise linker complains, revisit
uintptr_t guest_flash_vaddr;
uint64_t guest_flash_size;
uintptr_t guest_high_ram_vaddr;
uint64_t guest_high_ram_size;

linux_x86_setup_ret_t linux_setup;

void init(void)
{
    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", microkit_name);
    /* Place all the binaries in the right locations before starting the guest */
    size_t kernel_size = _guest_kernel_image_end - _guest_kernel_image;
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;
    size_t dsdt_aml_size = _guest_dsdt_aml_end - _guest_dsdt_aml;

    if (!linux_setup_images(guest_ram_vaddr, guest_ram_size, (uintptr_t)_guest_kernel_image, kernel_size,
                            (uintptr_t)_guest_initrd_image, initrd_size, _guest_dsdt_aml, dsdt_aml_size, GUEST_CMDLINE,
                            &linux_setup)) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }

    vcpu_set_up_long_mode(linux_setup.pml4_gpa, linux_setup.gdt_gpa, linux_setup.gdt_limit);

    /* Set up the virtual PCI bus */
    assert(pci_x86_init());

    /* Pass through COM1 serial port */
    microkit_vcpu_x86_enable_ioport(GUEST_BOOT_VCPU_ID, com1_ioport_id, com1_ioport_addr, com1_ioport_size);
    microkit_irq_ack(com1_ioport_id);

    /* Retrieve the TSC frequency from hardware */
    x86_host_tsc_t tsc_metadata = get_host_tsc(TIMER_DRV_CH_FOR_LAPIC);
    if (!tsc_metadata.valid) {
        LOG_VMM_ERR("cannot retrieve TSC frequency\n");
        return;
    }

    /* Initialise the virtual Local and I/O APICs */
    bool success = virq_controller_init(tsc_metadata.freq_hz, guest_vapic_vaddr);
    if (!success) {
        LOG_VMM_ERR("Failed to initialise virtual IRQ controller\n");
        return;
    }

    /* Pass through serial IRQs */
    assert(virq_ioapic_register_passthrough(0, 4, com1_ioport_id));

    guest_start(linux_setup.kernel_entry_gpa, 0, 0);
}

void notified(microkit_channel ch)
{
    switch (ch) {
    case TIMER_DRV_CH_FOR_LAPIC: {
        handle_lapic_timer_nftn(GUEST_BOOT_VCPU_ID);
        break;
    }
    case TIMER_DRV_CH_FOR_HPET_CH0:
    case TIMER_DRV_CH_FOR_HPET_CH1:
    case TIMER_DRV_CH_FOR_HPET_CH2:
        hpet_handle_timer_ntfn(ch);
        break;
    default:
        if (!virq_ioapic_handle_passthrough(ch)) {
            LOG_VMM_ERR("failed to passthrough IRQ ch %d\n", ch);
        };
    }
}

/*
 * The primary purpose of the VMM after initialisation is to act as a fault-handler.
 * Whenever our guest causes an exception, it gets delivered to this entry point for
 * the VMM to handle.
 */
seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo)
{
    uint64_t new_rip;
    bool success = fault_handle(child, &new_rip);
    if (success) {
        microkit_vcpu_x86_deferred_resume(new_rip, VMCS_PCC_DEFAULT, 0);
    }

    return seL4_True;
}
