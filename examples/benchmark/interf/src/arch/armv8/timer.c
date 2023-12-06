#include <timer.h>
#include <sysregs.h>

uint64_t TIMER_FREQ;

void timer_set(uint64_t n)
{
    uint64_t current = MRS(CNTVCT_EL0);
    MSR(CNTV_CVAL_EL0, current + n);
    MSR(CNTV_CTL_EL0, 1);
}

uint64_t timer_get()
{
    uint64_t time = MRS(CNTVCT_EL0);
    return time; // assumes plat_freq = 100MHz
}
