#include <microkit.h>
#include <stdbool.h>
#include <stddef.h>
#include <libvmm/virq.h>

// TODO: move out of here
#ifdef CONFIG_PLAT_QEMU_RISCV_VIRT
#define PLIC_ADDR 0xc000000
#define PLIC_SIZE 0x4000000
#elif CONFIG_PLAT_CHESHIRE
#define PLIC_ADDR 0x4000000
#define PLIC_SIZE 0x4000000
#else
#error "Unknown platform for PLIC"
#endif

bool plic_handle_fault(size_t vcpu_id, size_t offset, seL4_Word fsr, seL4_UserContext *regs);

bool plic_inject_timer_irq(size_t vcpu_id);

bool plic_inject_irq(size_t vcpu_id, int irq);
bool plic_register_irq(size_t vcpu_id, size_t irq, virq_ack_fn_t ack_fn, void *ack_data);
