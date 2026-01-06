#include <stdbool.h>
#include <stdint.h>
#include <sel4/sel4.h>

bool emulate_qemu_fw_cfg(seL4_VCPUContext *vctx, bool is_read, uint16_t port_addr);
