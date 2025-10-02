/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2025, UNSW (ABN 57 195 873 179)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*
 * This component controls and maintains the GIC for the VM.
 * IRQs must be registered at init time with vm_virq_new(...)
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

#include <stdint.h>

#include <string.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/aarch64/fault.h>
#include <libvmm/arch/aarch64/vgic/vgic.h>
#include <libvmm/arch/aarch64/vgic/virq.h>
#include <libvmm/arch/aarch64/vgic/vgic_v3.h>
#include <libvmm/arch/aarch64/vgic/vdist.h>

vgic_t vgic;

static bool vgic_handle_fault_redist_read(size_t vcpu_id, vgic_t *vgic, uint64_t offset, uint64_t fsr,
                                          seL4_UserContext *regs)
{
    size_t target_vcpu_id = offset / GIC_REDIST_INDIVIDUAL_SIZE;
    uint64_t real_offset = offset % GIC_REDIST_INDIVIDUAL_SIZE;

    assert(target_vcpu_id < GUEST_NUM_VCPUS);
    assert(vcpu_id < GUEST_NUM_VCPUS);

    struct gic_redist_map *gic_redist = vgic_get_redist(vgic->registers, target_vcpu_id);
    struct gic_redist_sgi_ppi_map *gic_redist_sgi_ppi = vgic_get_redist_sgi_ppi(vgic->registers, target_vcpu_id);
    uint64_t reg = 0;

    switch (real_offset) {
    case RANGE32(GICR_CTLR, GICR_CTLR):
        reg = gic_redist->ctlr;
        break;
    case RANGE32(GICR_IIDR, GICR_IIDR):
        reg = gic_redist->iidr;
        break;
    case RANGE32(GICR_TYPER, GICR_TYPER):
        reg = gic_redist->typer;
        break;
    case RANGE32(GICR_WAKER, GICR_WAKER):
        reg = gic_redist->waker;
        break;
    case RANGE32(GICR_PIDR2, GICR_PIDR2):
        reg = gic_redist->pidr2;
        break;
    case RANGE32(GICR_IGROUPR0, GICR_IGROUPR0):
        reg = gic_redist_sgi_ppi->igroupr0;
    case RANGE32(GICR_ICFGR1, GICR_ICFGR1):
        reg = gic_redist_sgi_ppi->icfgrn_rw;
        break;
    default:
        LOG_VMM_ERR("Unknown vgic redist register read offset 0x%x from vcpu %lu for vcpu %lu\n", real_offset, vcpu_id,
                    target_vcpu_id);
        assert(false);
        // @ivanv: used to be ignore_fault, double check this is right
        bool success = fault_advance_vcpu(vcpu_id, regs);
        // @ivanv: todo error handling
        assert(success);
    }

    uint64_t mask = fault_get_data_mask(offset, fsr);
    fault_emulate_write(regs, offset, fsr, reg & mask);
    // @ivanv: todo error handling

    return true;
}

static bool vgic_handle_fault_redist_write(size_t vcpu_id, vgic_t *vgic, uint64_t offset, uint64_t fsr,
                                           seL4_UserContext *regs)
{
    size_t target_vcpu_id = offset / GIC_REDIST_INDIVIDUAL_SIZE;
    uint64_t real_offset = offset % GIC_REDIST_INDIVIDUAL_SIZE;

    assert(vcpu_id < GUEST_NUM_VCPUS);
    assert(target_vcpu_id < GUEST_NUM_VCPUS);

    struct gic_redist_sgi_ppi_map *gic_redist_sgi_ppi = vgic_get_redist_sgi_ppi(vgic->registers, target_vcpu_id);

    uint32_t mask = fault_get_data_mask(offset, fsr);
    uint32_t data;

    switch (real_offset) {
    case RANGE32(GICR_WAKER, GICR_WAKER):
        /* Writes are ignored */
        break;
    case RANGE32(GICR_IGROUPR0, GICR_IGROUPR0):
        emulate_reg_write_access(regs, offset, fsr, &(gic_redist_sgi_ppi->igroupr0));
        break;
    case RANGE32(GICR_ISENABLER0, GICR_ISENABLER0):
        data = fault_get_data(regs, fsr) & mask;
        while (data) {
            int irq;
            irq = CTZ(data);
            data &= ~(1U << irq);
            set_sgi_ppi_enable_v3(gic_redist_sgi_ppi, irq, true);
        }
        break;
    case RANGE32(GICR_ICENABLER0, GICR_ICENABLER0):
        data = fault_get_data(regs, fsr) & mask;
        while (data) {
            int irq;
            irq = CTZ(data);
            data &= ~(1U << irq);
            set_sgi_ppi_enable_v3(gic_redist_sgi_ppi, irq, false);
        }
        break;
    case RANGE32(GICR_ICACTIVER0, GICR_ICACTIVER0):
        data = fault_get_data(regs, fsr) & mask;
        while (data) {
            int irq;
            irq = CTZ(data);
            data &= ~(1U << irq);
            set_sgi_ppi_active_v3(gic_redist_sgi_ppi, irq, false);
        }
        break;
    case RANGE32(GICR_IPRIORITYR0, GICR_IPRIORITYRN):
        // @billn: revisit, this register sets the priority for SGIs and PPIs, this is an opportunity for us to
        // invoke microkit_vcpu_arm_inject_irq in inject code path with the correct priority rather than just using zero.
        // The `GICR_IPRIORITYR<n>` have 8x 32-bits registers, we need to access the correct one based on the offset faulted.
        assert((real_offset - GICR_IPRIORITYR0) / 4 < 8);
        emulate_reg_write_access(regs, offset, fsr,
                                 &(gic_redist_sgi_ppi->ipriorityrn[(real_offset - GICR_IPRIORITYR0) / 4]));
        break;
    default:
        LOG_VMM_ERR("Unknown vgic redist register write offset 0x%x from vcpu %lu for vcpu %lu, value: 0x%lx\n",
                    real_offset, vcpu_id, target_vcpu_id, fault_get_data(regs, fsr));
        assert(false);
    }

    return true;
}

bool vgic_handle_fault_redist(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data)
{
    if (fault_is_read(fsr)) {
        return vgic_handle_fault_redist_read(vcpu_id, &vgic, offset, fsr, regs);
    } else {
        return vgic_handle_fault_redist_write(vcpu_id, &vgic, offset, fsr, regs);
    }
}

#define GICD_TYPER (0x7B04A0 | ITLINES)

static void vgic_dist_reset(struct gic_dist_map *dist)
{
    memset(dist, 0, sizeof(*dist));

    dist->typer            = GICD_TYPER; /* RO */
    dist->iidr             = 0x1043B ; /* RO */

    dist->enable_set[0]    = 0x0000ffff; /* 16bit RO */
    dist->enable_clr[0]    = 0x0000ffff; /* 16bit RO */

    dist->config[0]        = 0xaaaaaaaa; /* RO */

    dist->pidrn[0]         = 0x44;     /* RO */
    dist->pidrn[4]         = 0x92;     /* RO */
    dist->pidrn[5]         = 0xB4;     /* RO */
    dist->pidrn[6]         = 0x3B;     /* RO */

    dist->cidrn[0]         = 0x0D;     /* RO */
    dist->cidrn[1]         = 0xF0;     /* RO */
    dist->cidrn[2]         = 0x05;     /* RO */
    dist->cidrn[3]         = 0xB1;     /* RO */
}

static void vgic_redist_reset(struct gic_redist_map *redist, int vcpu_id, bool is_last_vcpu)
{
    memset(redist, 0, sizeof(*redist));
    redist->typer = 0; /* RO */

    if (is_last_vcpu) {
        // if this is the last vcpu in sequence, mark this redistributor frame as the last.
        redist->typer |= BIT_LOW(4);
    }

    // set processor number
    uint64_t proc_num_mask = vcpu_id << 8;
    redist->typer |= proc_num_mask;

    // set vcpu affinity number
    uint64_t aff0_mask = (uint64_t)vcpu_id << 32;
    redist->typer |= aff0_mask;

    redist->iidr            = 0x1143B;  /* RO */

    redist->pidr0           = 0x93;     /* RO */
    redist->pidr1           = 0xB4;     /* RO */
    redist->pidr2           = 0x3B;     /* RO */
    redist->pidr4           = 0x44;     /* RO */

    redist->cidr0           = 0x0D;     /* RO */
    redist->cidr1           = 0xF0;     /* RO */
    redist->cidr2           = 0x05;     /* RO */
    redist->cidr3           = 0xB1;     /* RO */
}

static void vgic_redist_sgi_ppi_reset(struct gic_redist_sgi_ppi_map *redist_sgi_ppi)
{
    memset(redist_sgi_ppi, 0, sizeof(*redist_sgi_ppi));
}

// @ivanv: come back to
struct gic_dist_map dist;
struct gic_redist_map redist[GUEST_NUM_VCPUS];
struct gic_redist_sgi_ppi_map redist_sgi_ppi[GUEST_NUM_VCPUS];
vgic_reg_t vgic_regs;

void vgic_init()
{
    // @ivanv: audit
    for (int i = 0; i < NUM_SLOTS_SPI_VIRQ; i++) {
        vgic.vspis[i].virq = VIRQ_INVALID;
    }
    for (int vcpu = 0; vcpu < GUEST_NUM_VCPUS; vcpu++) {
        for (int i = 0; i < NUM_VCPU_LOCAL_VIRQS; i++) {
            vgic.vgic_vcpu[vcpu].local_virqs[i].virq = VIRQ_INVALID;
        }
        for (int i = 0; i < NUM_LIST_REGS; i++) {
            vgic.vgic_vcpu[vcpu].lr_shadow[i].virq = VIRQ_INVALID;
        }

        vgic_redist_reset(&redist[vcpu], vcpu, vcpu == GUEST_NUM_VCPUS - 1);
        vgic_redist_sgi_ppi_reset(&redist_sgi_ppi[vcpu]);

        vgic_regs.redist[vcpu] = &redist[vcpu];
        vgic_regs.sgi[vcpu] = &redist_sgi_ppi[vcpu];
    }

    vgic.registers = &vgic_regs;
    vgic_regs.dist = &dist;

    vgic_dist_reset(&dist);
}
