#include <libvmm/util/util.h>
#include <libvmm/arch/x86_64/util.h>
#include <libvmm/arch/x86_64/vmcs.h>
#include <libvmm/arch/x86_64/qemu_fw_cfg.h>

/* See references:
 * https://wiki.osdev.org/QEMU_fw_cfg for reference.
 * https://www.qemu.org/docs/master/specs/fw_cfg.html.
 */

#define FW_CFG_PORT_SEL     0x510
#define FW_CFG_PORT_DATA    0x511
#define FW_CFG_PORT_DMA     0x514

#define FW_CFG_SIGNATURE    0x0000
#define FW_CFG_ID           0x0001
#define FW_CFG_FILE_DIR     0x0019

/* Currently selected port to control. */
uint16_t port_sel;

const char *fw_cfg_signature = "QEMU";
// TODO: maybe find a better way of state tracking.
size_t sig_idx = 0;

// static bool emulate_qemu_fw_cfg_signature(seL4_VCPUContext *vctx, bool is_read) {
//     if (is_read) {
//         vctx->eax = fw_cfg_signature[sig_idx];
//         // TODO
//         if (sig_idx >= 4) {
//             vctx->eax = 0;
//         } else {
//             sig_idx++;
//         }

//         return true;
//     } else {
//         return false;
//     }
// }

// static bool emulate_qemu_fw_cfg_

bool emulate_qemu_fw_cfg(seL4_VCPUContext *vctx, uint16_t port_addr, bool is_read, bool is_string, ioport_access_width_t access_width) {
    switch (port_addr) {
    case FW_CFG_PORT_SEL:
        if (!is_read) {
            port_sel = vctx->eax;
            LOG_VMM("fw cfg port_sel is 0x%x\n", port_sel);
        } else {
            /* FW_CFG_PORT_SEL is write-only register. */
            return false;
        }
        break;
    case FW_CFG_PORT_DATA:
        assert(access_width == IOPORT_BYTE_ACCESS_QUAL);

        if (is_read && is_string) {
            switch (port_sel) {
            case FW_CFG_SIGNATURE:
                assert(sig_idx < strlen(fw_cfg_signature));

                uint64_t dest_gpa;
                int _bytes_to_page_boundary;
                assert(gva_to_gpa(0, vctx->edi, &dest_gpa, &_bytes_to_page_boundary));
                char *dest = gpa_to_vaddr(dest_gpa);
                dest[0] = fw_cfg_signature[sig_idx];
                sig_idx++;

                // "After the byte, word, or doubleword is transfer from the I/O
                // port to the memory location, the DI/EDI/RDI register is incremented
                // or decremented automatically according to the setting of the DF flag
                // in the EFLAGS register. (If the DF flag is 0, the (E)DI register is incremented;
                // if the DF flag is 1, the (E)DI register is decremented.) The (E)DI register is
                // incremented or decremented by 1 for byte operations, by 2 for word operations,
                // or by 4 for doubleword operations."

                uint64_t eflags = microkit_vcpu_x86_read_vmcs(0, VMX_GUEST_RFLAGS);
                if (eflags & BIT(10)) {
                    vctx->edi -= ioports_access_width_to_bytes(access_width);
                } else {
                    vctx->edi += ioports_access_width_to_bytes(access_width);
                }

                break;
            default:
                return false;
            }
        } else if (is_read && !is_string) {
            LOG_VMM_ERR("implement me\n");
            return false;
        } else {
            /* FW_CFG_PORT_DATA is read-only. */
            return false;
        }
    }

    return true;
}
