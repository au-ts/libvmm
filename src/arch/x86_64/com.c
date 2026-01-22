#include <libvmm/arch/x86_64/com.h>
#include <libvmm/util/util.h>
#include <sddf/util/util.h>
#include <microkit.h>

static uint8_t com_scratch[4];

void emulate_com(seL4_VCPUContext *vctx, size_t idx, size_t reg_offset, bool is_read) {
    assert(idx < sizeof(com_scratch) / sizeof(uint8_t));
    switch (reg_offset) {
    case 0x7: {
        if (is_read) {
            vctx->eax = com_scratch[idx];
        } else {
            com_scratch[idx] = vctx->eax;
        }
        break;
    }
    case 0x0: {
        // com2 FIFO
        if (is_read) {
            vctx->eax = 0;
        } else {
            // LOG_VMM("com2 out: %c\n", vctx->eax);
            microkit_dbg_putc(vctx->eax);
        }
        break;
    }
    case 0x4:
        // Modem Control Register
        break;
    case 0x2: {
        if (is_read) {
            // Interrupt Identification Register
            vctx->eax = 0;
        } else {
            // First In First Out Control Register
            LOG_VMM("com2 FIFO write %x\n", vctx->eax);
        }
        break;
    }
    case 0x5: {
        // Line Status Register
        if (is_read) {
            vctx->eax = BIT(5) | BIT(6);
        } else {
            assert(false);
        }
        break;
    }
    case 0x3: {
        // Line Control Register
        if (is_read) {
            vctx->eax = BIT(7);
        } else {
            LOG_VMM("com2 LCR write %x\n", vctx->eax);
        }
        break;
    }
    case 0x6:
        // Modem Status Register
        break;
    default:
        // LOG_VMM_ERR("unknown COM offset 0%lx\n", reg_offset);
        break;
    }
}
