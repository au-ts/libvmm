#include <wfi.h>

extern void shmem_test();
extern void shmem_benchmark_notification();

void main(void){

    shmem_benchmark_notification();

    while(1) wfi();
}
