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

#include <libvmm/arch/x86_64/uefi.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/hpet.h>
#include <libvmm/arch/x86_64/pci.h>
#include <libvmm/arch/x86_64/pit.h>
#include <libvmm/arch/x86_64/virq.h>
#include <libvmm/arch/x86_64/vcpu.h>

#include <sddf/util/custom_libc/string.h>

// @billn sus, use package asm script
#include "board/x86_64_generic_vtx/simple_dsdt.hex"

uint64_t com1_ioport_id;
uint64_t com1_ioport_addr;
uint64_t com1_ioport_size = 8;

uint64_t com2_ioport_id;
uint64_t com2_ioport_addr;
uint64_t com2_ioport_size = 8;

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

/* Data for the guest's UEFI firmware image. */
extern char _guest_firmware_image[];
extern char _guest_firmware_image_end[];

/* Microkit will set this variable to the start of the guest RAM memory region. */
uintptr_t guest_ram_vaddr;
uint64_t guest_ram_size;
uintptr_t guest_flash_vaddr;
uint64_t guest_flash_size;

bool tsc_calibrating = true;
uint64_t tsc_pre, tsc_post, measured_tsc_hz;

void init(void)
{
    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", microkit_name);

    size_t firm_size = _guest_firmware_image_end - _guest_firmware_image;
    assert(uefi_setup_images(guest_ram_vaddr, guest_ram_size, guest_flash_vaddr, guest_flash_size,
                             _guest_firmware_image, firm_size, simple_dsdt_aml_code, sizeof(simple_dsdt_aml_code)));

    vcpu_set_up_reset_state();

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

            guest_start(0xfff0, 0, 0);
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
