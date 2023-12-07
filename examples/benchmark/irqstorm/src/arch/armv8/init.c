
#include <stdint.h>
#include <cpu.h>
#include <psci.h>
#include <gic.h>
#include <timer.h>
#include <sysregs.h>

void _start();

__attribute__((weak))
void arch_init(){
    uint64_t cpuid = get_cpuid();
    gic_init();
    TIMER_FREQ = MRS(CNTFRQ_EL0);
#ifndef SINGLE_CORE
    if(cpu_is_master()){
        size_t i = cpuid + 1;
        int ret;
        do {
            ret = psci_cpu_on(i, (uintptr_t) _start, 0);
        } while(i++, ret == PSCI_E_SUCCESS);
    }
#endif
    asm volatile("MSR   DAIFClr, #2\n\t");
}
