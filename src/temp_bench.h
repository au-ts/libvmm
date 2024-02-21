#pragma once

#include "sel4bench.h"

enum timed_event
{
    ERROR,
    VMFault,
    UnknownSyscall,
    UserException,
    VGICMaintenance,
    VCPUFault,
    VPPIEvent,
    TestingEvent,

};

enum sel4_timed_syscall
{
    TCB_WriteRegisters,
    TCB_ReadRegisters,
    Microkit_Ack_VPPI,
    Microkit_ARM_VCPU_Inject,
    Microkit_ARM_VCPU_Ack,
    Microkit_Reply,
    LibVMM_Total_Event,
};

typedef struct bench_event_
{
    ccnt_t cycles;
    enum timed_event event;
    enum sel4_timed_syscall syscall;
} bench_event_t;

void add_event(ccnt_t cycles, enum timed_event event, enum sel4_timed_syscall syscall);
void display_results();