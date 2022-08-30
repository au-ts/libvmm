#include <stdbool.h>
#include <stdint.h>
#include <sel4cp.h>

bool advance_vcpu_fault(seL4_UserContext *regs, uint32_t hsr);
