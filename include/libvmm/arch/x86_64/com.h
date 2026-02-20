#include <stdbool.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/ioports.h>

void emulate_com(seL4_VCPUContext *vctx, size_t idx, size_t reg_offset, bool is_read, bool is_rep, bool is_string, ioport_access_width_t access_width);
