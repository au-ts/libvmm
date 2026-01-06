#include <stdbool.h>
#include <stdint.h>
#include <sel4/sel4.h>
#include <libvmm/arch/x86_64/ioports.h>

bool emulate_qemu_fw_cfg(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read, bool is_string, ioport_access_width_t access_width);
