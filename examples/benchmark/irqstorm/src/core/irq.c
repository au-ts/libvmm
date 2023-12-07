#include <irq.h>
#include <stddef.h>

irq_handler_t irq_handlers[IRQ_NUM]; 

void irq_set_handler(unsigned id, irq_handler_t handler){
    if(id < IRQ_NUM)
        irq_handlers[id] = handler;
}

void irq_handle(unsigned id){
    if(id < IRQ_NUM && irq_handlers[id] != NULL)
        irq_handlers[id](id);
}
