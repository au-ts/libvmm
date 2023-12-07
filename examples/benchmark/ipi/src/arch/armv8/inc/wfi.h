#ifndef WFI_H
#define WFI_H

static inline void wfi(){
    asm volatile("wfi\n\t" ::: "memory");
}

#endif