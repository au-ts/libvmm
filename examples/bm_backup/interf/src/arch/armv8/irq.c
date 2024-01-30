#include <irq.h>
#include <cpu.h>
#include <gic.h>

#ifndef GIC_VERSION
#error "GIC_VERSION not defined for this platform"
#endif

void irq_enable(unsigned id) {
   gic_set_enable(id, true); 
   if(GIC_VERSION == GICV2) {
       gic_set_trgt(id, gic_get_trgt(id) | (1 << get_cpuid()));
   } else {
       gic_set_route(id, get_cpuid());
   }
}

void irq_set_prio(unsigned id, unsigned prio){
    gic_set_prio(id, (uint8_t) prio);
}

void irq_send_ipi(uint64_t target_cpu_mask) {
    for(int i = 0; i < sizeof(target_cpu_mask)*8; i++) {
        if(target_cpu_mask & (1ull << i)) {
            gic_send_sgi(i, IPI_IRQ_ID);
        }
    }
}
