/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*
 * This file controls and maintains the virtualised GIC (V2) for the VM. The spec used is: @ivanv
 * IRQs must be registered at initialisation using the function: vm_register_irq
 *
 * @ivanv: talk about what features this driver actually implements
 * This driver supports multiple vCPUs.
 *
 * This function creates and registers an IRQ data structure which will be used for IRQ maintenance
 * b) ENABLING: When the VM enables the IRQ, it checks the pending flag for the VM.
 *   - If the IRQ is not pending, we either
 *        1) have not received an IRQ so it is still enabled in seL4
 *        2) have received an IRQ, but ignored it because the VM had disabled it.
 *     In either case, we simply ACK the IRQ with seL4. In case 1), the IRQ will come straight through,
       in case 2), we have ACKed an IRQ that was not yet pending anyway.
 *   - If the IRQ is already pending, we can assume that the VM has yet to ACK the IRQ and take no further
 *     action.
 *   Transitions: b->c
 * c) PIRQ: When an IRQ is received from seL4, seL4 disables the IRQ and sends an async message. When the VMM
 *    receives the message.
 *   - If the IRQ is enabled, we set the pending flag in the VM and inject the appropriate IRQ
 *     leading to state d)
 *   - If the IRQ is disabled, the VMM takes no further action, leading to state b)
 *   Transitions: (enabled)? c->d :  c->b
 * d) When the VM acknowledges the IRQ, an exception is raised and delivered to the VMM. When the VMM
 *    receives the exception, it clears the pending flag and acks the IRQ with seL4, leading back to state c)
 *    Transition: d->c
 * g) When/if the VM disables the IRQ, we may still have an IRQ resident in the GIC. We allow
 *    this IRQ to be delivered to the VM, but subsequent IRQs will not be delivered as seen by state c)
 *    Transitions g->c
 *
 *   NOTE: There is a big assumption that the VM will not manually manipulate our pending flags and
 *         destroy our state. The affects of this will be an IRQ that is never acknowledged and hence,
 *         will never occur again.
 */

#include "vgic.h"

#include <stdint.h>
#include <sel4cp.h>

// #include "../fault.h"
#include "virq.h"
#include "vgic_v2.h"
#include "vdist.h"
#include "../util.h"

// @ivanv: do we really need to store size?
struct vgic_dist_device {
    uint64_t pstart;
    uint64_t size;
    vgic_t *vgic;
};

/* The driver expects the VGIC state to be initialised before calling any of the driver functionality. */
static vgic_t vgic;
static struct vgic_dist_device vgic_dist;

static void vgic_dist_reset(struct gic_dist_map *gic_dist)
{
    gic_dist = {0};
    gic_dist->ic_type         = 0x0000fce7; /* RO */
    gic_dist->dist_ident      = 0x0200043b; /* RO */

    for (int i = 0; i < CONFIG_MAX_NUM_NODES; i++) {
        gic_dist->enable_set0[i]   = 0x0000ffff; /* 16bit RO */
        gic_dist->enable_clr0[i]   = 0x0000ffff; /* 16bit RO */
    }

    /* Reset value depends on GIC configuration */
    gic_dist->config[0]       = 0xaaaaaaaa; /* RO */
    gic_dist->config[1]       = 0x55540000;
    gic_dist->config[2]       = 0x55555555;
    gic_dist->config[3]       = 0x55555555;
    gic_dist->config[4]       = 0x55555555;
    gic_dist->config[5]       = 0x55555555;
    gic_dist->config[6]       = 0x55555555;
    gic_dist->config[7]       = 0x55555555;
    gic_dist->config[8]       = 0x55555555;
    gic_dist->config[9]       = 0x55555555;
    gic_dist->config[10]      = 0x55555555;
    gic_dist->config[11]      = 0x55555555;
    gic_dist->config[12]      = 0x55555555;
    gic_dist->config[13]      = 0x55555555;
    gic_dist->config[14]      = 0x55555555;
    gic_dist->config[15]      = 0x55555555;

    /* Configure per-processor SGI/PPI target registers */
    for (int i = 0; i < CONFIG_MAX_NUM_NODES; i++) {
        for (int j = 0; j < ARRAY_SIZE(gic_dist->targets0[i]); j++) {
            for (int irq = 0; irq < sizeof(uint32_t); irq++) {
                gic_dist->targets0[i][j] |= ((1 << i) << (irq * 8));
            }
        }
    }
    /* Deliver the SPI interrupts to the first CPU interface */
    for (int i = 0; i < ARRAY_SIZE(gic_dist->targets); i++) {
        gic_dist->targets[i] = 0x1010101;
    }

    /* identification */
    gic_dist->periph_id[4]    = 0x00000004; /* RO */
    gic_dist->periph_id[8]    = 0x00000090; /* RO */
    gic_dist->periph_id[9]    = 0x000000b4; /* RO */
    gic_dist->periph_id[10]   = 0x0000002b; /* RO */
    gic_dist->component_id[0] = 0x0000000d; /* RO */
    gic_dist->component_id[1] = 0x000000f0; /* RO */
    gic_dist->component_id[2] = 0x00000005; /* RO */
    gic_dist->component_id[3] = 0x000000b1; /* RO */
}

bool vm_register_irq(size_t vcpu_id, int irq, irq_ack_fn_t ack_fn, void *cookie)
{
    struct virq_handle virq = {
        .virq = irq,
        .ack = ack_fn,
        .ack_data = cookie,
    };

    int err = virq_add(vcpu_id, &vgic, virq_data);
    return err;
}

int vgic_inject_irq(uint64_t vcpu_id, int irq)
{
    DIRQ("Injecting IRQ %d\n", irq);

    int err = vgic_dist_set_pending_irq(&vgic, vcpu_id, irq);

    // @ivanv: explain why we don't check error before checking this fault stuff
    // @ivanv: seperately, it seems weird to have this fault handling code here?
    // @ivanv: revist
    // if (!fault_handled(vcpu->vcpu_arch.fault) && fault_is_wfi(vcpu->vcpu_arch.fault)) {
    //     ignore_fault(vcpu->vcpu_arch.fault);
    // }

    return err;
}

// @ivanv: do we need this?
static bool handle_vgic_vcpu_fault(vm_t *vm, vm_vcpu_t *vcpu, uintptr_t fault_addr,
                                                    size_t fault_length,
                                                    void *cookie)
{
    /* We shouldn't fault on the vgic vcpu region as it should be mapped in
     * with all rights */
    return false;
}

bool vgic_init()
{
    /* Distributor */
    /* @ivanv: we probably don't need the vgic_dist_device abstraction anyways right? */
    vgic_dist.pstart = GIC_DIST_PADDR;
    vgic_dist.size = GIC_DIST_SIZE;
    vgic_dist.vgic = &vgic;

    // @ivanv: So here we're doing more mapping, should we just do this statically?
    // we should get rid of this then I guess...
    // vm_memory_reservation_t *vgic_dist_res = vm_reserve_memory_at(vm, GIC_DIST_PADDR, PAGE_SIZE_4K,
    //                                                               handle_vgic_dist_fault, (void *)vgic_dist);
    // vgic_dist_reset(vgic_dist);

    // /* Remap VCPU to CPU */
    // vm_memory_reservation_t *vgic_vcpu_reservation = vm_reserve_memory_at(vm, GIC_CPU_PADDR, PAGE_SIZE_4K,
    //                                                                       handle_vgic_vcpu_fault, NULL);
    // int err = vm_map_reservation(vm, vgic_vcpu_reservation, vgic_vcpu_iterator, (void *)vm);
    // if (err) {
    //     free(vgic_dist->vgic);
    //     return -1;
    // }

    return true;
}

// int handle_vgic_maintenance(int idx)
// {
//     /* STATE d) */
//     vgic_vcpu_t *vgic_vcpu = get_vgic_vcpu(vgic, 0);
//     halt(vgic_vcpu);
//     halt((idx >= 0) && (idx < ARRAY_SIZE(vgic_vcpu->lr_shadow)));
//     virq_handle_t *slot = &vgic_vcpu->lr_shadow[idx];
//     halt(*slot);
//     virq_handle_t lr_virq = *slot;
//     *slot = NULL;
//     /* Clear pending */
//     DIRQ("Maintenance IRQ %d\n", lr_virq->virq);
//     set_pending(vgic->dist, lr_virq->virq, false, vcpu->vcpu_id);
//     virq_ack(vcpu, lr_virq);

//     /* Check the overflow list for pending IRQs */
//     struct virq_handle *virq = vgic_irq_dequeue(vgic, vcpu);
//     if (virq) {
//         return vgic_vcpu_load_list_reg(vgic, vcpu, idx, 0, virq);
//     }

//     return 0;
// }

bool handle_vgic_maintenance()
{
    int err;
    int idx = sel4cp_mr_get(seL4_VGICMaintenance_IDX);
    /* Currently not handling spurious IRQs */
    // @ivanv: wtf, this comment seems irrelevant to the code.
    halt(idx >= 0);

    // int err = handle_vgic_maintenance(idx);
    vgic_vcpu_t *vgic_vcpu = get_vgic_vcpu(vgic, VCPU_ID);
    halt(vgic_vcpu);
    halt((idx >= 0) && (idx < ARRAY_SIZE(vgic_vcpu->lr_shadow)));
    virq_handle_t *slot = &vgic_vcpu->lr_shadow[idx];
    halt(*slot);
    virq_handle_t lr_virq = *slot;
    *slot = NULL;
    /* Clear pending */
    DIRQ("Maintenance IRQ %d\n", lr_virq->virq);
    set_pending(vgic->dist, lr_virq->virq, false, VCPU_ID);
    virq_ack(VCPU_ID, lr_virq);
    /* Check the overflow list for pending IRQs */
    struct virq_handle *virq = vgic_irq_dequeue(&vgic, VCPU_ID);
    if (virq) {
        err = vgic_vcpu_load_list_reg(&vgic, VCPU_ID, idx, 0, virq);
    }
    if (!err) {
        // seL4_MessageInfo_t reply;
        // reply = sel4cp_msginfo_new(0, 0);
        // seL4_Reply(reply);
    } else {
        ZF_LOGF("vGIC maintenance handler failed (error %d)", err);
        return false;
    }

    return true;
}
