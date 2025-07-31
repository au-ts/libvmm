#include <libvmm/fault.h>

bool fault_is_read(seL4_Word fsr) {
    assert(false);
    return false;
}

bool fault_is_write(seL4_Word fsr) {
    assert(false);
    return false;
}

bool fault_handle(size_t vcpu_id, microkit_msginfo msginfo) {
    assert(false);
    return false;
}
