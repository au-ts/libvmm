#include <stdint.h>
#include <stdio.h>
#include <csrs.h>
#include <plic.h>
#include <irq.h>

static bool is_external(uint64_t cause) {
    switch(cause) {
        case SCAUSE_CODE_SEI:
        case SCAUSE_CODE_UEI:
            return true;
        default:
            return false;
    }
}

__attribute__((interrupt("supervisor"), aligned(4)))
void exception_handler(){
    
    uint64_t scause = CSRR(scause);
    if(is_external(scause)) {
        plic_handle();
    } else { 
       uint64_t id = (scause & ~(1ull << 63)) + 1024;
       irq_handle(id);
       if(id == IPI_IRQ_ID) {
           CSRC(sip, SIP_SSIE);
       }
    }
}
