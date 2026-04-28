#include <libvmm/libvmm.h>
#include <sddf/serial/queue.h>
#include <sddf/serial/config.h>

#define REG_UARTDR  0x0
#define REG_UARTFR  0x18
#define REG_TCR     0x30

struct pl011_regs {
    uint32_t tcr;
};

struct pl011_regs pl011_regs;

extern serial_queue_handle_t serial_rx_queue;
extern serial_queue_handle_t serial_tx_queue;

extern serial_client_config_t serial_config;

bool pl011_access_read(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data) {
    uint32_t reg = 0;
    switch (offset) {
    case REG_TCR:
        reg = pl011_regs.tcr;
        break;
    case REG_UARTFR:
        reg = (1 << 7);
        break;
    default:
        // LOG_VMM_ERR("todo offset: 0x%lx\n", offset);
        break;
    }

    uint32_t mask = fault_get_data_mask(offset, fsr);
    fault_emulate_write(regs, offset, fsr, reg & mask);

    return true;
}

bool pl011_access_write(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *cb_data) {
    uint32_t data = fault_get_data(regs, fsr);
    uint32_t mask = fault_get_data_mask(offset, fsr);
    /* Mask the data to write */
    data &= mask;

    switch (offset) {
    case REG_UARTDR: {
        microkit_dbg_putc(data);
        // serial_enqueue_batch(&serial_tx_queue, 1, &data);
        // microkit_notify(serial_config.tx.id);
        break;
    }
    default:
        // LOG_VMM_ERR("todo offset: 0x%lx\n", offset);
        break;
    }
    return true;
}

bool pl011_access(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data) {
    if (fault_is_read(fsr)) {
        return pl011_access_read(vcpu_id, offset, fsr, regs, data);
    } else {
        return pl011_access_write(vcpu_id, offset, fsr, regs, data);
    }
}

void pl011_init(uint64_t gpa) {
    bool success = fault_register_vm_exception_handler(gpa, 0x100, pl011_access, NULL);
    assert(success);
}
