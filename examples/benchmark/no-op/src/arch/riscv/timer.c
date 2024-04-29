#include <sbi.h>

void timer_enable()
{
}

uint64_t timer_get()
{
    uint64_t time;
    asm volatile("rdtime %0" : "=r"(time)); 
    return time;
}

void timer_set(uint64_t n)
{
    sbi_set_timer(timer_get() + n);
}
