#include <libvmm/util/util.h>
#include <stdbool.h>

void emulate_com(seL4_VCPUContext *vctx, size_t idx, size_t reg_offset, bool is_read);
