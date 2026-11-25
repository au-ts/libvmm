/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2022, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <microkit.h>
#include <stdbool.h>
#include <stdint.h>
#include <libvmm/virq.h>

#if defined(CONFIG_ARM_GIC_V3_SUPPORT)
#define GIC_V3
#else
#define GIC_V2
#endif

#if defined(GIC_V2)
#define GIC_DIST_SIZE 0x1000
#elif defined(GIC_V3)
#define GIC_DIST_SIZE                0x10000
#define GIC_REDIST_INDIVIDUAL_SIZE   0x20000
#define GIC_REDIST_TOTAL_SIZE        (GIC_REDIST_INDIVIDUAL_SIZE * GUEST_MAX_NUM_VCPUS)
#else
#error Unknown GIC version
#endif

/* Uncomment these defines for more verbose logging in the GIC driver. */
// #define DEBUG_IRQ
// #define DEBUG_DIST

#if defined(DEBUG_IRQ)
#define LOG_IRQ(...) do{ printf("VGIC|IRQ: "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_IRQ(...) do{}while(0)
#endif

#if defined(DEBUG_DIST)
#define LOG_DIST(...) do{ printf("VGIC|DIST: "); printf(__VA_ARGS__); }while(0)
#else
#define LOG_DIST(...) do{}while(0)
#endif

#define IRQ_IDX(irq) ((irq) / 32)
#define IRQ_BIT(irq) (1U << ((irq) % 32))

#if defined(GIC_V2)
void vgic_init(uint64_t gic_dist_gpa);
#elif defined(GIC_V3)
void vgic_init(uint64_t gic_dist_gpa, uint64_t gic_redist_gpa);
#endif

bool vgic_handle_fault_maintenance(size_t vcpu_id);

bool vgic_handle_fault_dist(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data);
bool vgic_handle_fault_redist(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data);

bool vgic_register_irq(size_t vcpu_id, int virq_num, virq_ack_fn_t ack_fn, void *ack_data);
bool vgic_inject_irq(size_t vcpu_id, int irq);
