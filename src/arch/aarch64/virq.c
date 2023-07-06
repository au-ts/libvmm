#include <sel4cp.h>
#include "vgic/vgic.h"
#include "../../util/util.h"
#include "../../virq.h"

#define SGI_RESCHEDULE_IRQ  0
#define SGI_FUNC_CALL       1
#define PPI_VTIMER_IRQ      27

static void vppi_event_ack(size_t vcpu_id, int irq, void *cookie)
{
    sel4cp_arm_vcpu_ack_vppi(vcpu_id, irq);
}

static void sgi_ack(size_t vcpu_id, int irq, void *cookie) {}

bool virq_controller_init(size_t boot_vcpu_id) {
    vgic_init();
#if defined(GIC_V2)
    LOG_VMM("initialised virtual GICv2 driver\n");
#elif defined(GIC_V3)
    LOG_VMM("initialised virtual GICv3 driver\n");
#else
#error "Unsupported GIC version"
#endif

    bool success = vgic_register_irq(boot_vcpu_id, PPI_VTIMER_IRQ, &vppi_event_ack, NULL);
    if (!success) {
        LOG_VMM_ERR("Failed to register vCPU virtual timer IRQ: 0x%lx\n", PPI_VTIMER_IRQ);
        return false;
    }
    success = vgic_register_irq(boot_vcpu_id, SGI_RESCHEDULE_IRQ, &sgi_ack, NULL);
    if (!success) {
        LOG_VMM_ERR("Failed to register vCPU SGI 0 IRQ");
        return false;
    }
    success = vgic_register_irq(boot_vcpu_id, SGI_FUNC_CALL, &sgi_ack, NULL);
    if (!success) {
        LOG_VMM_ERR("Failed to register vCPU SGI 1 IRQ");
        return false;
    }

    return true;
}

bool virq_inject(size_t vcpu_id, int irq) {
    return vgic_inject_irq(vcpu_id, irq);
}

bool virq_register(size_t vcpu_id, size_t virq_num, virq_ack_fn_t ack_fn, void *ack_data) {
    return vgic_register_irq(vcpu_id, virq_num, ack_fn, ack_data);
}
