#ifndef IRQ_H
#define IRQ_H

#include <stdint.h>
#include <stdbool.h>
#include <arch/irq.h>

typedef void (*irq_handler_t)(unsigned id);

void irq_handle(unsigned id);
void irq_set_handler(unsigned id, irq_handler_t handler);
void irq_enable(unsigned id);
void irq_set_prio(unsigned id, unsigned prio);
void irq_send_ipi(uint64_t target_cpu_mask);
bool irq_get_pend(unsigned id);
void irq_clear_pend(unsigned id);

#endif // IRQ_H
