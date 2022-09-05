#include <sel4cp.h>
#include "vgic.h"
#include "virq.h"
#include "../util/util.h"
#include "../fault.h"

// @ivanv: TEMP!
#if defined(GIC_V2)
#include "vgic_v2.h"
#elif defined(GIC_V3)
#include "vgic_v3.h"
#endif

#include "vdist.h"

/* The driver expects the VGIC state to be initialised before calling any of the driver functionality. */
extern vgic_t vgic;

bool handle_vgic_maintenance()
{
    // @ivanv: reivist, also inconsistency between int and bool
    int err;
    int idx = sel4cp_mr_get(seL4_VGICMaintenance_IDX);
    /* Currently not handling spurious IRQs */
    // @ivanv: wtf, this comment seems irrelevant to the code.
    assert(idx >= 0);

    // @ivanv: Revisit and make sure it's still correct.
    vgic_vcpu_t *vgic_vcpu = get_vgic_vcpu(&vgic, VCPU_ID);
    assert(vgic_vcpu);
    assert((idx >= 0) && (idx < ARRAY_SIZE(vgic_vcpu->lr_shadow)));
    struct virq_handle *slot = &vgic_vcpu->lr_shadow[idx];
    assert(slot->virq != VIRQ_INVALID);
    struct virq_handle lr_virq = *slot;
    slot->virq = VIRQ_INVALID;
    slot->ack_fn = NULL;
    slot->ack_data = NULL;
    /* Clear pending */
    LOG_IRQ("Maintenance IRQ %d\n", lr_virq.virq);
    set_pending(vgic_get_dist(vgic.registers), lr_virq.virq, false, VCPU_ID);
    virq_ack(VCPU_ID, &lr_virq);
    /* Check the overflow list for pending IRQs */
    struct virq_handle *virq = vgic_irq_dequeue(&vgic, VCPU_ID);

#if defined(GIC_V2)
    int group = 0;
#elif defined(GIC_V3)
    int group = 1;
#else
#error "Unknown GIC version"
#endif

    if (virq) {
        err = vgic_vcpu_load_list_reg(&vgic, VCPU_ID, idx, group, virq);
    }
    if (!err) {
        reply_to_fault();
    } else {
        printf("VGIC|ERROR: maintenance handler failed (error %d)", err);
        assert(0);
        return false;
    }

    return true;
}

// @ivanv: maybe this shouldn't be here?
bool vgic_register_irq(uint64_t vcpu_id, int virq_num, irq_ack_fn_t ack_fn, void *ack_data) {
    assert(virq_num >= 0 && virq_num != VIRQ_INVALID);
    struct virq_handle virq = {
        .virq = virq_num,
        .ack_fn = ack_fn,
        .ack_data = ack_data,
    };

    return virq_add(vcpu_id, &vgic, &virq);
}

bool vgic_inject_irq(uint64_t vcpu_id, int irq)
{
    LOG_IRQ("Injecting IRQ %d\n", irq);

    return vgic_dist_set_pending_irq(&vgic, vcpu_id, irq);

    // @ivanv: explain why we don't check error before checking this fault stuff
    // @ivanv: seperately, it seems weird to have this fault handling code here?
    // @ivanv: revist
    // if (!fault_handled(vcpu->vcpu_arch.fault) && fault_is_wfi(vcpu->vcpu_arch.fault)) {
    //     // ignore_fault(vcpu->vcpu_arch.fault);
    //     err = advance_vcpu_fault(regs);
    // }
}

// @ivanv: revisit this whole function
bool handle_vgic_dist_fault(uint64_t vcpu_id, uint64_t fault_addr, uint64_t fsr, seL4_UserContext *regs)
{
    // @ivanv: revisit this comment
    /* There is a fault object per VCPU with much more context, the parameters
     * fault_addr and fault_length are no longer used.
     */
    // fault_t *fault = vcpu->vcpu_arch.fault;
    // assert(fault);
    // assert(fault_addr == fault_get_address(vcpu->vcpu_arch.fault));

    // assert(cookie);
    // struct vgic_dist_device *d = (typeof(d))cookie;
    // vgic_t *vgic = d->vgic;
    // assert(vgic->dist);

    assert(fault_addr >= GIC_DIST_PADDR);
    uint64_t offset = fault_addr - GIC_DIST_PADDR;
    // @ivanv: this isn't necessarily one page?
    // assert(offset < PAGE_SIZE_4K);

    if (fault_is_read(fsr)) {
        // printf("VGIC|INFO: Read dist\n");
        return vgic_dist_reg_read(vcpu_id, &vgic, offset, fsr, regs);
    } else {
        // printf("VGIC|INFO: Write dist\n");
        return vgic_dist_reg_write(vcpu_id, &vgic, offset, fsr, regs);
    }
}