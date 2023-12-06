#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>
#include <arch/irq.h>

typedef void (*irq_handler_t)(unsigned id);

void irq_handle(unsigned id);
void irq_set_handler(unsigned id, irq_handler_t handler);
void irq_enable(unsigned id);
void irq_set_prio(unsigned id, unsigned prio);
void irq_send_ipi(uint64_t target_cpu_mask);

#endif // IRQ_H
