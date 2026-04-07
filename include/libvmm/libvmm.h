/*
 * Copyright 2026, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include <sel4/sel4.h>

#include <libvmm/config.h>
#include <libvmm/guest.h>
#include <libvmm/guest_ram.h>
#include <libvmm/tcb.h>
#include <libvmm/vcpu.h>
#include <libvmm/virq.h>

#include <libvmm/virtio/virtio.h>
#include <libvmm/virtio/block.h>
#include <libvmm/virtio/console.h>
#include <libvmm/virtio/net.h>
#include <libvmm/virtio/sound.h>

#if defined(CONFIG_ARCH_ARM)
#include <libvmm/arch/aarch64/linux.h>
#include <libvmm/arch/aarch64/fault.h>
#include <libvmm/arch/aarch64/smc.h>
#elif defined(CONFIG_ARCH_X86)
#include <libvmm/arch/x86_64/acpi.h>
#include <libvmm/arch/x86_64/apic.h>
#include <libvmm/arch/x86_64/com.h>
#include <libvmm/arch/x86_64/cpuid.h>
#include <libvmm/arch/x86_64/e820.h>
#include <libvmm/arch/x86_64/fault.h>
#include <libvmm/arch/x86_64/hpet.h>
#include <libvmm/arch/x86_64/instruction.h>
#include <libvmm/arch/x86_64/ioports.h>
#include <libvmm/arch/x86_64/linux.h>
#include <libvmm/arch/x86_64/msr.h>
#include <libvmm/arch/x86_64/pci.h>
#include <libvmm/arch/x86_64/util.h>
#include <libvmm/arch/x86_64/vcpu.h>
#include <libvmm/arch/x86_64/virq.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/tsc.h>
#include <libvmm/arch/x86_64/memory_space.h>
#endif
