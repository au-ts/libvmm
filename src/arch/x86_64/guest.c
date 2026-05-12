/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <libvmm/libvmm.h>
#include <libvmm/guest.h>

#if defined(CONFIG_ARCH_X86_64)
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/cpuid.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/msr.h>
#include <libvmm/arch/x86_64/pci.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/linux.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/vmm_state.h>
#include <sel4/arch/vmenter.h>
#endif

bool guest_init(arch_guest_init_t init_args)
{
    if (!initialise_local_and_global_vmm_state(init_args.bsp, init_args.global_vmm_state_ptr)) {
        LOG_VMM_ERR("failed to initialise VMM state.\n");
        return false;
    }

    /* Set up the virtual PCI bus */
    if (!pci_x86_init()) {
        LOG_VMM_ERR("failed to initialise virtual PCI bus.\n");
        return false;
    }

    /* Initialise guest time library */
    if (!initialise_guest_time(init_args.timer_ch)) {
        LOG_VMM_ERR("failed to initialise guest time keeper.\n");
        return false;
    }

    /* Initialise CPUID */
    if (!initialise_cpuid()) {
        LOG_VMM_ERR("failed to initialise CPUID\n");
        return false;
    }

    /* Initialise MSRs */
    if (!initialise_msrs(init_args.bsp)) {
        LOG_VMM_ERR("failed to initialise MSRs\n");
        return false;
    }

    /* Initialise the virtual Local and I/O APICs */
    bool success = virq_controller_init(guest_time_tsc_hz(), 0);
    if (!success) {
        LOG_VMM_ERR("Failed to initialise virtual IRQ controllers\n");
        return false;
    }

    return true;
}

bool guest_start_long_mode(uint64_t kernel_rip, uint64_t cr3, uint64_t gdt_gpa, uint64_t gdt_limit,
                           seL4_VCPUContext *initial_regs)
{
    if (!vcpu_set_up_long_mode(cr3, gdt_gpa, gdt_limit)) {
        LOG_VMM_ERR("Failed to set up virtual CPU\n");
        return false;
    }

    /* Write out the initial CPU registers. This is required when there is some sort
       of ABI between the guest software being booted and us acting as the "bootloader".
       For example, Linux expects the GPA of the "zero page" to be in RSI when the CPU
       is resumed. */
    microkit_vcpu_x86_write_regs(GUEST_BOOT_VCPU_ID, initial_regs);

    LOG_VMM("starting guest at 0x%lx\n", kernel_rip);
    microkit_mr_set(SEL4_VMENTER_CALL_EIP_MR, kernel_rip);
    microkit_mr_set(SEL4_VMENTER_CALL_CONTROL_PPC_MR, VMCS_PPVC_DEFAULT);
    microkit_mr_set(SEL4_VMENTER_CALL_INTERRUPT_INFO_MR, 0);
    microkit_vcpu_x86_deferred_resume();
    return true;
}
