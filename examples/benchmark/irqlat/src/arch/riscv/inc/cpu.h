#ifndef CPU_H
#define CPU_H

#include <stdint.h>
#include <stdbool.h>

extern int primary_hart;

static inline uint64_t get_cpuid(){
    register uint64_t hartid asm("tp");
    return hartid;
}

static inline bool cpu_is_master(){
    return get_cpuid() == primary_hart;
}

#endif
