#include <microkit.h>
#include "sel4bench.h"
#include "temp_bench.h"
#include "util.h"


#define NUM_EVENTS 2000

#if defined(BENCH_vmm)
#define RUN 1
#else
#define RUN 0
#endif

bench_event_t events[NUM_EVENTS];
int event_count = 0;
int event_loop_count = 0;

void add_event(ccnt_t cycles, enum timed_event event, enum sel4_timed_syscall syscall) {
    if (!RUN) {
        return;
    }
    events[event_count].cycles = cycles;
    events[event_count].event = event;
    events[event_count].syscall = syscall;
    event_count++;
    if (event_count >= NUM_EVENTS) {
        event_count = 0;
        event_loop_count++;
    }
}

void display_results() {
    if (!RUN)
    {
        return;
    }
    printf("START_RESULTS\n");
    ccnt_t total_k_cycles = 0;
    for (int i = 0; i < event_count; i++) {
        // Format print nicely
        printf("| Event: ");
        switch (events[i].event) {
            case VMFault:
                printf("VMFault         ");
                break;
            case UnknownSyscall:
                printf("UnknownSyscall  ");
                break;
            case UserException: 
                printf("UserException   ");
                break;
            case VGICMaintenance:   
                printf("VGICMaintenance ");
                break;
            case VCPUFault: 
                printf("VCPUFault       ");
                break;
            case VPPIEvent: 
                printf("VPPIEvent       ");
                break;
            case TestingEvent:
                printf("TestingEvent    ");
                break;
            default:
                printf("ERROR           ");
                break;
            
        }
        printf(" | Syscall: ");
        switch (events[i].syscall) {
            case TCB_WriteRegisters:
                printf("TCB_WriteRegisters       ");
                break;
            case TCB_ReadRegisters:
                printf("TCB_ReadRegisters        ");
                break;
            case Microkit_Ack_VPPI:
                printf("Microkit_Ack_VPPI        ");
                break;
            case Microkit_ARM_VCPU_Inject:
                printf("Microkit_ARM_VCPU_Inject ");
                break;
            case Microkit_ARM_VCPU_Ack:
                printf("Microkit_ARM_VCPU_Ack    ");
                break;
            case Microkit_Reply:
                printf("Microkit_Reply           ");
                break;
            case LibVMM_Total_Event:
                printf("LibVMM_Total_Event       ");
                break;
            default:
                printf("ERROR                    ");
                break;
        }
        printf(" | Cycles: %lu |\n", events[i].cycles);
        total_k_cycles += events[i].cycles;

    }
    printf("END_RESULTS\n");

    printf("\tNum events: %d. Event loop count: %d\n", event_count, event_loop_count);
    printf("\tTotal cycles: %lu\n", total_k_cycles);


}
