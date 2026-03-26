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

// @billn sus, use package asm script
#include "board/x86_64_generic_vtx/simple_dsdt.hex"

uint64_t com1_ioport_id;
uint64_t com1_ioport_addr;
uint64_t com1_ioport_size = 8;

#define COM1_IRQ_CH 0

#define GUEST_CMDLINE "earlyprintk=serial,0x3f8,115200 debug console=ttyS0,115200 earlycon=serial,0x3f8,115200 loglevel=8"

/* Data for the guest's kernel image. */
extern char _guest_kernel_image[];
extern char _guest_kernel_image_end[];
/* Data for the initial RAM disk to be passed to the kernel. */
extern char _guest_initrd_image[];
extern char _guest_initrd_image_end[];
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

    if (!linux_setup_images(guest_ram_vaddr, guest_ram_size, (uintptr_t)_guest_kernel_image, kernel_size,
                            (uintptr_t)_guest_initrd_image, initrd_size, simple_dsdt_aml_code,
                            sizeof(simple_dsdt_aml_code), GUEST_CMDLINE, &linux_setup)) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }

    vcpu_set_up_long_mode(linux_setup.pml4_gpa, linux_setup.gdt_gpa, linux_setup.gdt_limit);

    /* Set up the virtual PCI bus */
    assert(pci_x86_init());

    /* Pass through COM1 serial port */
    microkit_vcpu_x86_enable_ioport(GUEST_BOOT_VCPU_ID, com1_ioport_id, com1_ioport_addr, com1_ioport_size);
    microkit_irq_ack(COM1_IRQ_CH);

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
    assert(virq_ioapic_register_passthrough(0, 4, COM1_IRQ_CH));

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
