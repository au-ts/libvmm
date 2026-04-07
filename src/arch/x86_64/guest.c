/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <libvmm/libvmm.h>
#include <libvmm/guest.h>

#if defined(CONFIG_ARCH_X86_64)
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/linux.h>
#include <libvmm/arch/x86_64/fault.h>
#include <sel4/arch/vmenter.h>
#endif

bool guest_start(uintptr_t kernel_rip, seL4_VCPUContext initial_regs)
{
    /* Write out the initial CPU registers. This is required when there is some sort
       of ABI between the guest software being booted and us acting as the "bootloader".
       For example, Linux expects the GPA of the "zero page" to be in RSI when the CPU
       is resumed. */
    microkit_vcpu_x86_write_regs(GUEST_BOOT_VCPU_ID, &initial_regs);

    LOG_VMM("starting guest at 0x%lx\n", kernel_rip);
    microkit_vcpu_x86_deferred_resume(kernel_rip, VMCS_PCC_DEFAULT, 0);
    return true;
}
