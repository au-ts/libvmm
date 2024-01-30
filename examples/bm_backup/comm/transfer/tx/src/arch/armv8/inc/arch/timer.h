#ifndef ARCH_TIMER_H
#define ARCH_TIMER_H

#include <stdint.h>
#include <sysregs.h>

extern uint64_t TIMER_FREQ;

static inline void timer_disable()
{
    MSR(CNTV_CTL_EL0, 0);
}


static inline uint64_t timer_set(uint64_t n)
{
    MSR(CNTV_TVAL_EL0, n);
    MSR(CNTV_CTL_EL0, 1);
    return MRS(CNTV_CVAL_EL0);
}

static inline  uint64_t timer_get()
{
    uint64_t time = MRS(CNTPCT_EL0);
    return time; // assumes plat_freq = 100MHz
}

static inline  uint64_t timer_busy_wait(uint64_t n)
{
    uint64_t time = MRS(CNTVCT_EL0) + n;
    while (MRS(CNTVCT_EL0) < time);
}


#endif
