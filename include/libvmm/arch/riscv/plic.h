/*
 * Copyright 2025, UNSW
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <microkit.h>
#include <stdbool.h>
#include <stddef.h>
#include <libvmm/virq.h>

#if defined(CONFIG_PLAT_QEMU_RISCV_VIRT) || defined(CONFIG_PLAT_HIFIVE_P550)
#define PLIC_ADDR 0xc000000
#define PLIC_SIZE 0x4000000
#else
#error "Unknown platform for PLIC"
#endif

bool plic_handle_fault(size_t vcpu_id, size_t offset, seL4_Word fsr, seL4_UserContext *regs, void *data);

bool plic_inject_timer_irq(size_t vcpu_id);

bool plic_inject_irq(size_t vcpu_id, int irq);
bool plic_register_irq(size_t vcpu_id, size_t irq, virq_ack_fn_t ack_fn, void *ack_data);
