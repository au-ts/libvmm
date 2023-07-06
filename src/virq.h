#include <stddef.h>
#include <sel4cp.h>

bool virq_controller_init(size_t boot_vcpu_id);
bool virq_register(size_t vcpu_id, size_t virq_num, virq_ack_fn_t ack_fn, void *ack_data);
bool virq_inject(size_t vcpu_id, int irq);
