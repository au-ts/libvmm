/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <libvmm/guest.h>
#include <libvmm/util/util.h>
#include <sddf/timer/client.h>

#include <libvmm/arch/x86_64/linux.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/hpet.h>
#include <libvmm/arch/x86_64/pci.h>
#include <libvmm/arch/x86_64/pit.h>
#include <libvmm/arch/x86_64/virq.h>
#include <libvmm/arch/x86_64/vcpu.h>

// @billn sus, use package asm script
#include "board/x86_64_generic_vtx/simple_dsdt.hex"

uint64_t com1_ioport_id;
uint64_t com1_ioport_addr;
uint64_t com1_ioport_size = 8;

uint64_t primary_ata_cmd_pio_id;
uint64_t primary_ata_cmd_pio_addr;

uint64_t primary_ata_ctrl_pio_id;
uint64_t primary_ata_ctrl_pio_addr;

uint64_t second_ata_cmd_pio_id;
uint64_t second_ata_cmd_pio_addr;

uint64_t second_ata_ctrl_pio_id;
uint64_t second_ata_ctrl_pio_addr;

uint64_t pci_conf_addr_pio_id;
uint64_t pci_conf_addr_pio_addr;

uint64_t pci_conf_data_pio_id;
uint64_t pci_conf_data_pio_addr;

#define COM1_IRQ_CH 0
#define PRIM_ATA_IRQ_CH 1
#define SECD_ATA_IRQ_CH 2

#define GUEST_CMDLINE "pci=nocrs earlyprintk=serial,0x3f8,115200 debug console=ttyS0,115200 earlycon=serial,0x3f8,115200 loglevel=8"

/* Data for the guest's kernel image. */
extern char _guest_kernel_image[];
extern char _guest_kernel_image_end[];
/* Data for the initial RAM disk to be passed to the kernel. */
extern char _guest_initrd_image[];
extern char _guest_initrd_image_end[];
/* Microkit will set this variable to the start of the guest RAM memory region. */
uintptr_t guest_ram_vaddr;

// @billn unused, but have to leave it here otherwise linker complains, revisit
uint64_t guest_ram_size;
uintptr_t guest_flash_vaddr;
uint64_t guest_flash_size;


bool tsc_calibrating = true;
linux_x86_setup_ret_t linux_setup;
uint64_t tsc_pre, tsc_post, measured_tsc_hz;

void init(void)
{
    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", microkit_name);
    /* Place all the binaries in the right locations before starting the guest */
    size_t kernel_size = _guest_kernel_image_end - _guest_kernel_image;

    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;
    if (!linux_setup_images(guest_ram_vaddr, 0x10000000, (uintptr_t)_guest_kernel_image, kernel_size,
                            (uintptr_t)_guest_initrd_image, initrd_size, simple_dsdt_aml_code,
                            sizeof(simple_dsdt_aml_code), GUEST_CMDLINE, &linux_setup)) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }

    vcpu_set_up_long_mode(linux_setup.pml4_gpa, linux_setup.gdt_gpa, linux_setup.gdt_limit);

    /* Pass through COM1 serial port and IDE disk controller */
    microkit_vcpu_x86_enable_ioport(GUEST_BOOT_VCPU_ID, com1_ioport_id, com1_ioport_addr, com1_ioport_size);
    passthrough_ide_controller(primary_ata_cmd_pio_id, primary_ata_cmd_pio_addr, primary_ata_ctrl_pio_id,
                               primary_ata_ctrl_pio_addr, second_ata_cmd_pio_id, second_ata_cmd_pio_addr,
                               second_ata_ctrl_pio_id, second_ata_ctrl_pio_addr);

    microkit_irq_ack(COM1_IRQ_CH);
    microkit_irq_ack(PRIM_ATA_IRQ_CH);
    microkit_irq_ack(SECD_ATA_IRQ_CH);

    LOG_VMM("Measuring TSC frequency...\n");
    sddf_timer_set_timeout(TIMER_DRV_CH_FOR_LAPIC, NS_IN_S);
    tsc_pre = rdtsc();
}

void notified(microkit_channel ch)
{
    switch (ch) {
    case TIMER_DRV_CH_FOR_LAPIC: {
        if (tsc_calibrating) {
            tsc_post = rdtsc();
            measured_tsc_hz = tsc_post - tsc_pre;
            LOG_VMM("TSC frequency is %lu Hz\n", measured_tsc_hz);
            tsc_calibrating = false;

            /* Initialise the virtual APIC */
            bool success = virq_controller_init(measured_tsc_hz);
            if (!success) {
                LOG_VMM_ERR("Failed to initialise virtual IRQ controller\n");
                return;
            }

            /* Pass through IDE disk controller IRQs */
            assert(virq_ioapic_register_passthrough(0, 14, PRIM_ATA_IRQ_CH));
            assert(virq_ioapic_register_passthrough(0, 15, SECD_ATA_IRQ_CH));

            /* Pass through serial IRQs */
            assert(virq_ioapic_register_passthrough(0, 4, COM1_IRQ_CH));

            guest_start(linux_setup.kernel_entry_gpa, 0, 0);
        } else {
            handle_lapic_timer_nftn(GUEST_BOOT_VCPU_ID);
        }
        break;
    }
    case TIMER_DRV_CH_FOR_PIT:
        pit_handle_timer_ntfn();
        break;
    case TIMER_DRV_CH_FOR_HPET_CH0:
    case TIMER_DRV_CH_FOR_HPET_CH1:
    case TIMER_DRV_CH_FOR_HPET_CH2:
        hpet_handle_timer_ntfn(ch);
        break;
    default:
        // printf("Unexpected channel, ch: 0x%lx\n", ch);
        virq_ioapic_handle_passthrough(ch);
    }
}

/*
 * The primary purpose of the VMM after initialisation is to act as a fault-handler.
 * Whenever our guest causes an exception, it gets delivered to this entry point for
 * the VMM to handle.
 */
// seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo)
// {
// #if defined(CONFIG_ARCH_AARCH64)
//     bool success = fault_handle(child, msginfo);
//     if (success) {
//         /* Now that we have handled the fault successfully, we reply to it so
//          * that the guest can resume execution. */
//         *reply_msginfo = microkit_msginfo_new(0, 0);
//         return seL4_True;
//     }
// #endif

//     return seL4_False;
// }
