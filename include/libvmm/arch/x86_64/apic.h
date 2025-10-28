#include <stdint.h>
#include <stdbool.h>
#include <sel4/sel4.h>

bool apic_fault_handle(uint64_t offset, seL4_Word qualification);
