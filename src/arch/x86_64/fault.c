/*
 * Copyright 2025, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <libvmm/fault.h>

bool fault_is_read(seL4_Word fsr) {
    assert(false);
    return false;
}

bool fault_is_write(seL4_Word fsr) {
    assert(false);
    return false;
}

char *fault_to_string(seL4_Word fault_label)
{
    switch (fault_label) {
    case seL4_Fault_VMFault:
        return "virtual memory exception";
    case seL4_Fault_UserException:
        return "user exception";
    default:
        return "unknown fault";
    }
}

void dump_user_exception(void) {
    LOG_VMM("user exception fault with:\n");
    LOG_VMM("  PC: 0x%lx\n", microkit_mr_get(seL4_UserException_FaultIP));
    LOG_VMM("  SP: 0x%lx\n", microkit_mr_get(seL4_UserException_SP));
    LOG_VMM("  FLAGS: 0x%lx\n", microkit_mr_get(seL4_UserException_FLAGS));
    LOG_VMM("  Number: 0x%lx\n", microkit_mr_get(seL4_UserException_Number));
    LOG_VMM("  Code: 0x%lx\n", microkit_mr_get(seL4_UserException_Code));
}

bool fault_handle(size_t vcpu_id, microkit_msginfo msginfo) {
    size_t label = microkit_msginfo_get_label(msginfo);
    LOG_VMM("handling fault '%s', at PC 0x%lx\n", fault_to_string(label), microkit_mr_get(seL4_UserException_FaultIP));
    dump_user_exception();
    return false;
}