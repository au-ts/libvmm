/** 
 * Bao, a Lightweight Static Partitioning Hypervisor 
 *
 * Copyright (c) Bao Project (www.bao-project.org), 2019-
 *
 * Authors:
 *      Jose Martins <jose.martins@bao-project.org>
 *      Sandro Pinto <sandro.pinto@bao-project.org>
 *
 * Bao is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details. 
 *
 */

#include "gic.h"
#include <irq.h>
#include <cpu.h>

volatile gicd_t* gicd = (void*)0xF9010000;
volatile gicc_t* gicc = (void*)0xF902f000;

static size_t gic_num_int(){
    return ((gicd->TYPER & BIT_MASK(GICD_TYPER_ITLINENUM_OFF, GICD_TYPER_ITLINENUM_LEN) >>
        GICD_TYPER_ITLINENUM_OFF) +1)*32;
}

void gicc_init(){

     for(int i =0; i < GIC_NUM_INT_REGS(GIC_CPU_PRIV); i++){
        /**
         * Make sure all private interrupts are not enabled, non pending,
         * non active.
         */
        //gicd->ICENABLER[i] = -1;
        //gicd->ICPENDR[i] = -1;
        //gicd->ICACTIVER[i] = -1;
    }

    /* Clear any pending SGIs. */
    for(int i = 0; i < GIC_NUM_SGI_REGS; i++){
        //gicd->CPENDSGIR[i] = -1;
    }

    for(int i = 0; i< GIC_NUM_TARGET_REGS(GIC_CPU_PRIV); i++){
       //gicd->IPRIORITYR[i] = -1;
    }

    for(int i = 0; i< GIC_NUM_PRIO_REGS(GIC_CPU_PRIV); i++){
       //gicd->IPRIORITYR[i] = -1;
    }

    gicc->PMR = -1;
    gicc->CTLR |= GICC_CTLR_EN_BIT;
    
}

void gicd_init(){

    size_t int_num = gic_num_int();

//    /* Bring distributor to known state */
//    for(int i = GIC_NUM_PRIVINT_REGS; i< GIC_NUM_INT_REGS(int_num); i++){
//        /**
//         * Make sure all interrupts are not enabled, non pending,
//         * non active.
//         */
//        gicd->ICENABLER[i] = -1;
//        gicd->ICPENDR[i] = -1;
//        gicd->ICACTIVER[i] = -1;
//    }
//
//    /* All interrupts have lowest priority possible by default */
//    for(int i = 0; i< GIC_NUM_PRIO_REGS(int_num); i++)
//        gicd->IPRIORITYR[i] = -1;
//
//    /* No CPU targets for any interrupt by default */
//    for(int i = 0; i< GIC_NUM_TARGET_REGS(int_num); i++)
//        gicd->ITARGETSR[i] = 0;
//
//    /* No CPU targets for any interrupt by default */
//    for(int i = 0; i< GIC_NUM_CONFIG_REGS(int_num); i++)
//        gicd->ICFGR[i] = 0xAAAAAAAA;

    /* No need to setup gicd->NSACR as all interrupts are  setup to group 1 */

    /* Enable distributor */
    gicd->CTLR |= GICD_CTLR_EN_BIT;
}

void gic_init() {
    if(get_cpuid() == 0) {
        gicd_init();
    }
    gicc_init();
}

void gic_set_enable(uint64_t int_id, bool en){
    
    uint64_t reg_ind = int_id/(sizeof(uint32_t)*8);
    uint64_t bit = (1UL << int_id%(sizeof(uint32_t)*8));

    if(en)
        gicd->ISENABLER[reg_ind] = bit;
    else
        gicd->ICENABLER[reg_ind] = bit;

}

void gic_set_trgt(uint64_t int_id, uint8_t trgt)
{
    uint64_t reg_ind = (int_id * GIC_TARGET_BITS) / (sizeof(uint32_t) * 8);
    uint64_t off = (int_id * GIC_TARGET_BITS) % (sizeof(uint32_t) * 8);
    uint32_t mask = ((1U << GIC_TARGET_BITS) - 1) << off;

    gicd->ITARGETSR[reg_ind] =
        (gicd->ITARGETSR[reg_ind] & ~mask) | ((trgt << off) & mask);
}

uint8_t gic_get_trgt(uint64_t int_id)
{
    uint64_t reg_ind = (int_id * GIC_TARGET_BITS) / (sizeof(uint32_t) * 8);
    uint64_t off = (int_id * GIC_TARGET_BITS) % (sizeof(uint32_t) * 8);
    uint32_t mask = ((1U << GIC_TARGET_BITS) - 1) << off;

    return (gicd->ITARGETSR[reg_ind] & mask) >> off;
}

void gic_send_sgi(uint64_t cpu_target, uint64_t sgi_num){
    gicd->SGIR   = (1UL << (GICD_SGIR_CPUTRGLST_OFF + cpu_target))
        | (sgi_num & GICD_SGIR_SGIINTID_MSK);
}

void gic_set_prio(uint64_t int_id, uint8_t prio){
    uint64_t reg_ind = (int_id*GIC_PRIO_BITS)/(sizeof(uint32_t)*8);
    uint64_t off = (int_id*GIC_PRIO_BITS)%((sizeof(uint32_t)*8));
    uint64_t mask = ((1 << GIC_PRIO_BITS)-1) << off;

    gicd->IPRIORITYR[reg_ind] = (gicd->IPRIORITYR[reg_ind] & ~mask) | 
        ((prio << off) & mask);
}

bool gic_is_pending(uint64_t int_id){

    uint64_t reg_ind = int_id/(sizeof(uint32_t)*8);
    uint64_t off = int_id%(sizeof(uint32_t)*8);

    return ((1U << off) & gicd->ISPENDR[reg_ind]) != 0;
}

void gic_set_pending(uint64_t int_id, bool pending){
    uint64_t reg_ind = int_id / (sizeof(uint32_t) * 8);
    uint64_t mask = 1U << int_id % (sizeof(uint32_t) * 8);

    if (pending) {
        gicd->ISPENDR[reg_ind] = mask;
    } else {
        gicd->ICPENDR[reg_ind] = mask;
    }   
}

bool gic_is_active(uint64_t int_id){

    uint64_t reg_ind = int_id/(sizeof(uint32_t)*8);
    uint64_t off = int_id%(sizeof(uint32_t)*8);

    return ((1U << off) & gicd->ISACTIVER[reg_ind]) != 0;
}

void gic_handle(){

    uint64_t ack = gicc->IAR;
    uint64_t id = ack & GICC_IAR_ID_MSK;
    uint64_t src = (ack & GICC_IAR_CPU_MSK) >> GICC_IAR_CPU_OFF;

    if(id >= 1022) return;

    irq_handle(id);
        
    gicc->EOIR = ack;
    
}
