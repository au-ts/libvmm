/**
 * Bao, a Lightweight Static Partitioning Hypervisor
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *      Angelo Ruocco <angeloruocco90@gmail.com>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details.
 *
 */

#include <gic.h>
#include <sysregs.h>
#include <cpu.h>
#include <spinlock.h>
#include <fences.h>
#include <irq.h>

volatile gicd_t* gicd = (void*)0xF9010000;
volatile gicr_t* gicr = (void*)0xF9020000;

static spinlock_t gicd_lock;
static spinlock_t gicr_lock;


inline uint64_t gic_num_irqs()
{
    uint32_t itlinenumber =
        bit_extract(gicd->TYPER, GICD_TYPER_ITLN_OFF, GICD_TYPER_ITLN_LEN);
    return 32 * itlinenumber + 1;
}


static inline void gicc_init()
{
    /* Enable system register interface i*/
    MSR(ICC_PMR_EL1, -1);
    MSR(ICC_CTLR_EL1, GICC_CTLR_EN_BIT);
    MSR(ICC_IGRPEN1_EL1, ICC_IGRPEN_EL1_ENB_BIT);
}

static inline void gicr_init()
{
    gicr[get_cpuid()].ICENABLER0 = -1;
    gicr[get_cpuid()].ICPENDR0 = -1;
    gicr[get_cpuid()].ICACTIVER0 = -1;

    for (int i = 0; i < GIC_NUM_PRIO_REGS(GIC_CPU_PRIV); i++) {
        gicr[get_cpuid()].IPRIORITYR[i] = -1;
    }
}

//void gicc_save_state(gicc_state_t *state)
//{
//    state->CTLR = MRS(ICC_CTLR_EL1);
//    state->PMR = MRS(ICC_PMR_EL1);
//    //state->IAR = MRS(ICC_IAR1_EL1);
//    state->BPR = MRS(ICC_BPR1_EL1);
//    //state->EOIR = MRS(ICC_EOIR1_EL1);
//    state->RPR = MRS(ICC_RPR_EL1);
//    state->HPPIR = MRS(ICC_HPPIR1_EL1);
//    state->priv_ISENABLER = gicr[get_cpuid()].ISENABLER0;
//
//    for (int i = 0; i < GIC_NUM_PRIO_REGS(GIC_CPU_PRIV); i++) {
//        state->priv_IPRIORITYR[i] = gicr[get_cpuid()].IPRIORITYR[i];
//    }
//
//    state->HCR = MRS(ICH_HCR_EL2);
//    for (int i = 0; i < gich_num_lrs(); i++) {
//        state->LR[i] = gich_read_lr(i);
//    }
//}
//
//void gicc_restore_state(gicc_state_t *state)
//{
//    MSR(ICC_CTLR_EL1, state->CTLR);
//    MSR(ICC_PMR_EL1, state->PMR);
//    MSR(ICC_BPR1_EL1, state->BPR);
//    //MSR(ICC_EOIR1_EL1, state->EOIR);
//    //MSR(ICC_IAR1_EL1, state->IAR);
//    MSR(ICC_RPR_EL1, state->RPR);
//    MSR(ICC_HPPIR1_EL1, state->HPPIR);
//    gicr[get_cpuid()].ISENABLER0 = state->priv_ISENABLER;
//
//    for (int i = 0; i < GIC_NUM_PRIO_REGS(GIC_CPU_PRIV); i++) {
//        gicr[get_cpuid()].IPRIORITYR[i] = state->priv_IPRIORITYR[i];
//    }
//
//    MSR(ICH_HCR_EL2, state->HCR);
//    for (int i = 0; i < gich_num_lrs(); i++) {
//        gich_write_lr(i, state->LR[i]);
//    }
//}

void gic_cpu_init()
{
    gicr_init();
    gicc_init();
}

void gicd_init()
{
    size_t int_num = gic_num_irqs();

    /* Bring distributor to known state */
    for (int i = GIC_NUM_PRIVINT_REGS; i < GIC_NUM_INT_REGS(int_num); i++) {
        /**
         * Make sure all interrupts are not enabled, non pending,
         * non active.
         */
        gicd->ICENABLER[i] = -1;
        gicd->ICPENDR[i] = -1;
        gicd->ICACTIVER[i] = -1;
    }

    /* All interrupts have lowest priority possible by default */
    for (int i = GIC_CPU_PRIV; i < GIC_NUM_PRIO_REGS(int_num); i++)
        gicd->IPRIORITYR[i] = -1;

    /* No CPU targets for any interrupt by default */
    for (int i = GIC_CPU_PRIV; i < GIC_NUM_TARGET_REGS(int_num); i++)
        gicd->ITARGETSR[i] = 0;

    /* ICFGR are platform dependent, lets leave them as is */

    /* No need to setup gicd->NSACR as all interrupts are  setup to group 1 */

    /* Enable distributor and affinity routing */
    gicd->CTLR |= GICD_CTLR_ARE_NS_BIT | GICD_CTLR_ENA_BIT;
}

void gic_init()
{
    gic_cpu_init();

    if (get_cpuid() == 0) {
        gicd_init();
    }

}

void gic_handle()
{
    uint64_t ack = MRS(ICC_IAR1_EL1);
    uint64_t id = ack & ((1UL << 24) -1);

    if (id >= 1022) return;

    irq_handle(id);

    MSR(ICC_EOIR1_EL1, ack);
    //MSR(ICC_DIR_EL1, ack);
}

uint64_t gicd_get_prio(uint64_t int_id)
{
    uint64_t reg_ind = GIC_PRIO_REG(int_id);
    uint64_t off = GIC_PRIO_OFF(int_id);

    spin_lock(&gicd_lock);

    uint64_t prio =
        gicd->IPRIORITYR[reg_ind] >> off & BIT_MASK(off, GIC_PRIO_BITS);

    spin_unlock(&gicd_lock);

    return prio;
}

void gicd_set_icfgr(uint64_t int_id, uint8_t cfg)
{
    spin_lock(&gicd_lock);

    uint64_t reg_ind = (int_id * GIC_CONFIG_BITS) / (sizeof(uint32_t) * 8);
    uint64_t off = (int_id * GIC_CONFIG_BITS) % (sizeof(uint32_t) * 8);
    uint64_t mask = ((1U << GIC_CONFIG_BITS) - 1) << off;

    gicd->ICFGR[reg_ind] = (gicd->ICFGR[reg_ind] & ~mask) | ((cfg << off) & mask);

    spin_unlock(&gicd_lock);
}

void gicd_set_prio(uint64_t int_id, uint8_t prio)
{
    uint64_t reg_ind = GIC_PRIO_REG(int_id);
    uint64_t off = GIC_PRIO_OFF(int_id);
    uint64_t mask = BIT_MASK(off, GIC_PRIO_BITS);

    spin_lock(&gicd_lock);

    gicd->IPRIORITYR[reg_ind] =
        (gicd->IPRIORITYR[reg_ind] & ~mask) | ((prio << off) & mask);

    spin_unlock(&gicd_lock);
}

enum int_state gicd_get_state(uint64_t int_id)
{
    uint64_t reg_ind = GIC_INT_REG(int_id);
    uint64_t mask = GIC_INT_MASK(int_id);

    spin_lock(&gicd_lock);

    enum int_state pend = (gicd->ISPENDR[reg_ind] & mask) ? PEND : 0;
    enum int_state act = (gicd->ISACTIVER[reg_ind] & mask) ? ACT : 0;

    spin_unlock(&gicd_lock);

    return pend | act;
}

static void gicd_set_pend(uint64_t int_id, bool pend)
{
    spin_lock(&gicd_lock);

    if (gic_is_sgi(int_id)) {
        uint64_t reg_ind = GICD_SGI_REG(int_id);
        uint64_t off = GICD_SGI_OFF(int_id);

        if (pend) {
            gicd->SPENDSGIR[reg_ind] = (1U) << (off + get_cpuid());
        } else {
            gicd->CPENDSGIR[reg_ind] = BIT_MASK(off, 8);
        }
    } else {
        uint64_t reg_ind = GIC_INT_REG(int_id);

        if (pend) {
            gicd->ISPENDR[reg_ind] = GIC_INT_MASK(int_id);
        } else {
            gicd->ICPENDR[reg_ind] = GIC_INT_MASK(int_id);
        }
    }

    spin_unlock(&gicd_lock);
}

void gicd_set_act(uint64_t int_id, bool act)
{
    uint64_t reg_ind = GIC_INT_REG(int_id);

    spin_lock(&gicd_lock);

    if (act) {
        gicd->ISACTIVER[reg_ind] = GIC_INT_MASK(int_id);
    } else {
        gicd->ICACTIVER[reg_ind] = GIC_INT_MASK(int_id);
    }

    spin_unlock(&gicd_lock);
}

void gicd_set_state(uint64_t int_id, enum int_state state)
{
    gicd_set_act(int_id, state & ACT);
    gicd_set_pend(int_id, state & PEND);
}

void gicd_set_trgt(uint64_t int_id, uint8_t trgt)
{
    uint64_t reg_ind = GIC_TARGET_REG(int_id);
    uint64_t off = GIC_TARGET_OFF(int_id);
    uint32_t mask = BIT_MASK(off, GIC_TARGET_BITS);

    spin_lock(&gicd_lock);

    gicd->ITARGETSR[reg_ind] =
        (gicd->ITARGETSR[reg_ind] & ~mask) | ((trgt << off) & mask);

    spin_unlock(&gicd_lock);
}

void gicd_set_route(uint64_t int_id, uint64_t trgt)
{
    if (gic_is_priv(int_id)) return;

    gicd->IROUTER[int_id] = trgt;
}

void gicd_set_enable(uint64_t int_id, bool en)
{
    uint64_t bit = GIC_INT_MASK(int_id);

    uint64_t reg_ind = GIC_INT_REG(int_id);
    spin_lock(&gicd_lock);
    if (en)
        gicd->ISENABLER[reg_ind] = bit;
    else
        gicd->ICENABLER[reg_ind] = bit;
    spin_unlock(&gicd_lock);
}

void gicr_set_prio(uint64_t int_id, uint8_t prio, uint32_t gicr_id)
{
    uint64_t reg_ind = GIC_PRIO_REG(int_id);
    uint64_t off = GIC_PRIO_OFF(int_id);
    uint64_t mask = BIT_MASK(off, GIC_PRIO_BITS);

    spin_lock(&gicr_lock);

    gicr[gicr_id].IPRIORITYR[reg_ind] =
        (gicr[gicr_id].IPRIORITYR[reg_ind] & ~mask) | ((prio << off) & mask);

    spin_unlock(&gicr_lock);
}

uint64_t gicr_get_prio(uint64_t int_id, uint32_t gicr_id)
{
    uint64_t reg_ind = GIC_PRIO_REG(int_id);
    uint64_t off = GIC_PRIO_OFF(int_id);

    spin_lock(&gicr_lock);

    uint64_t prio =
        gicr[gicr_id].IPRIORITYR[reg_ind] >> off & BIT_MASK(off, GIC_PRIO_BITS);

    spin_unlock(&gicr_lock);

    return prio;
}

void gicr_set_icfgr(uint64_t int_id, uint8_t cfg, uint32_t gicr_id)
{
    spin_lock(&gicr_lock);

    uint64_t reg_ind = (int_id * GIC_CONFIG_BITS) / (sizeof(uint32_t) * 8);
    uint64_t off = (int_id * GIC_CONFIG_BITS) % (sizeof(uint32_t) * 8);
    uint64_t mask = ((1U << GIC_CONFIG_BITS) - 1) << off;

    if (reg_ind == 0) {
        gicr[gicr_id].ICFGR0 =
            (gicr[gicr_id].ICFGR0 & ~mask) | ((cfg << off) & mask);
    } else {
        gicr[gicr_id].ICFGR1 =
            (gicr[gicr_id].ICFGR1 & ~mask) | ((cfg << off) & mask);
    }

    spin_unlock(&gicr_lock);
}

enum int_state gicr_get_state(uint64_t int_id, uint32_t gicr_id)
{
    uint64_t mask = GIC_INT_MASK(int_id);

    spin_lock(&gicr_lock);

    enum int_state pend = (gicr[gicr_id].ISPENDR0 & mask) ? PEND : 0;
    enum int_state act = (gicr[gicr_id].ISACTIVER0 & mask) ? ACT : 0;

    spin_unlock(&gicr_lock);

    return pend | act;
}

static void gicr_set_pend(uint64_t int_id, bool pend, uint32_t gicr_id)
{
    spin_lock(&gicr_lock);
    if (pend) {
        gicr[gicr_id].ISPENDR0 = (1U) << (int_id);
    } else {
        gicr[gicr_id].ICPENDR0 = (1U) << (int_id);
    }
    spin_unlock(&gicr_lock);
}

void gicr_set_act(uint64_t int_id, bool act, uint32_t gicr_id)
{
    spin_lock(&gicr_lock);

    if (act) {
        gicr[gicr_id].ISACTIVER0 = GIC_INT_MASK(int_id);
    } else {
        gicr[gicr_id].ICACTIVER0 = GIC_INT_MASK(int_id);
    }

    spin_unlock(&gicr_lock);
}

void gicr_set_state(uint64_t int_id, enum int_state state, uint32_t gicr_id)
{
    gicr_set_act(int_id, state & ACT, gicr_id);
    gicr_set_pend(int_id, state & PEND, gicr_id);
}

void gicr_set_trgt(uint64_t int_id, uint8_t trgt, uint32_t gicr_id)
{
    spin_lock(&gicr_lock);

    spin_unlock(&gicr_lock);
}

void gicr_set_route(uint64_t int_id, uint8_t trgt, uint32_t gicr_id)
{
    gicr_set_trgt(int_id, trgt, gicr_id);
}

void gicr_set_enable(uint64_t int_id, bool en, uint32_t gicr_id)
{
    uint64_t bit = GIC_INT_MASK(int_id);

    spin_lock(&gicr_lock);
    if (en)
        gicr[gicr_id].ISENABLER0 = bit;
    else
        gicr[gicr_id].ICENABLER0 = bit;
    spin_unlock(&gicr_lock);
}

static bool irq_in_gicd(uint64_t int_id)
{
    if (int_id > 32 && int_id < 1025) return true;
    else return false;
}

void gic_send_sgi(uint64_t cpu_target, uint64_t sgi_num)
{
    if (sgi_num >= GIC_MAX_SGIS) return;
    
    uint64_t sgi = (1UL << (cpu_target & 0xffull)) | (sgi_num << 24);
    MSR(ICC_SGI1R_EL1, sgi); 
}

void gic_set_prio(uint64_t int_id, uint8_t prio)
{
    if (irq_in_gicd(int_id)) {
        return gicd_set_prio(int_id, prio);
    } else {
        return gicr_set_prio(int_id, prio, get_cpuid());
    }
}

uint64_t gic_get_prio(uint64_t int_id)
{
    if (irq_in_gicd(int_id)) {
        return gicd_get_prio(int_id);
    } else {
        return gicr_get_prio(int_id, get_cpuid());
    }
}

void gic_set_icfgr(uint64_t int_id, uint8_t cfg)
{
    if (irq_in_gicd(int_id)) {
        return gicd_set_icfgr(int_id, cfg);
    } else {
        return gicr_set_icfgr(int_id, cfg, get_cpuid());
    }
}

enum int_state gic_get_state(uint64_t int_id)
{
    if (irq_in_gicd(int_id)) {
        return gicd_get_state(int_id);
    } else {
        return gicr_get_state(int_id, get_cpuid());
    }
}

static void gic_set_pend(uint64_t int_id, bool pend)
{
    if (irq_in_gicd(int_id)) {
        return gicd_set_pend(int_id, pend);
    } else {
        return gicr_set_pend(int_id, pend, get_cpuid());
    }
}

void gic_set_act(uint64_t int_id, bool act)
{
    if (irq_in_gicd(int_id)) {
        return gicd_set_act(int_id, act);
    } else {
        return gicr_set_act(int_id, act, get_cpuid());
    }
}

void gic_set_state(uint64_t int_id, enum int_state state)
{
    if (irq_in_gicd(int_id)) {
        return gicd_set_state(int_id, state);
    } else {
        return gicr_set_state(int_id, state, get_cpuid());
    }
}

void gic_set_trgt(uint64_t int_id, uint8_t trgt)
{
    if (irq_in_gicd(int_id)) {
        return gicd_set_trgt(int_id, trgt);
    } else {
        return gicr_set_trgt(int_id, trgt, get_cpuid());
    }
}

void gic_set_route(uint64_t int_id, uint64_t trgt)
{
    return gicd_set_route(int_id, trgt);
}

void gic_set_enable(uint64_t int_id, bool en)
{
    if (irq_in_gicd(int_id)) {
        return gicd_set_enable(int_id, en);
    } else {
        return gicr_set_enable(int_id, en, get_cpuid());
    }
}

