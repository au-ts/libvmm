/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

//#define DEBUG_IRQ
//#define DEBUG_DIST

#ifdef DEBUG_IRQ
#define DIRQ(...) do{ sel4cp_dbg_puts("VGIC|IRQ: "); sel4cp_dbg_puts(__VA_ARGS__); }while(0)
#else
#define DIRQ(...) do{}while(0)
#endif

#ifdef DEBUG_DIST
#define DDIST(...) do{ sel4cp_dbg_puts("VGIC|DIST: "); sel4cp_dbg_puts(__VA_ARGS__); }while(0)
#else
#define DDIST(...) do{}while(0)
#endif

/* GIC Distributor register access utilities */
#define GIC_DIST_REGN(offset, reg) ((offset-reg)/sizeof(uint32_t))
#define RANGE32(a, b) a ... b + (sizeof(uint32_t)-1)

#define IRQ_IDX(irq) ((irq) / 32)
#define IRQ_BIT(irq) (1U << ((irq) % 32))


static inline void set_sgi_ppi_pending(struct gic_dist_map *gic_dist, int irq, bool set_pending, int vcpu_id)
{
    if (set_pending) {
        gic_dist->pending_set0[vcpu_id] |= IRQ_BIT(irq);
        gic_dist->pending_clr0[vcpu_id] |= IRQ_BIT(irq);
    } else {
        gic_dist->pending_set0[vcpu_id] &= ~IRQ_BIT(irq);
        gic_dist->pending_clr0[vcpu_id] &= ~IRQ_BIT(irq);
    }
}

static inline void set_spi_pending(struct gic_dist_map *gic_dist, int irq, bool set_pending)
{
    if (set_pending) {
        gic_dist->pending_set[IRQ_IDX(irq)] |= IRQ_BIT(irq);
        gic_dist->pending_clr[IRQ_IDX(irq)] |= IRQ_BIT(irq);
    } else {
        gic_dist->pending_set[IRQ_IDX(irq)] &= ~IRQ_BIT(irq);
        gic_dist->pending_clr[IRQ_IDX(irq)] &= ~IRQ_BIT(irq);
    }
}

static inline void set_pending(struct gic_dist_map *gic_dist, int irq, bool set_pending, int vcpu_id)
{
    if (irq < NUM_VCPU_LOCAL_VIRQS) {
        set_sgi_ppi_pending(gic_dist, irq, set_pending, vcpu_id);
        return;
    }
    set_spi_pending(gic_dist, irq, set_pending);
}

static inline bool is_sgi_ppi_pending(struct gic_dist_map *gic_dist, int irq, int vcpu_id)
{
    return !!(gic_dist->pending_set0[vcpu_id] & IRQ_BIT(irq));
}

static inline bool is_spi_pending(struct gic_dist_map *gic_dist, int irq)
{
    return !!(gic_dist->pending_set[IRQ_IDX(irq)] & IRQ_BIT(irq));
}

static inline bool is_pending(struct gic_dist_map *gic_dist, int irq, int vcpu_id)
{
    if (irq < NUM_VCPU_LOCAL_VIRQS) {
        return is_sgi_ppi_pending(gic_dist, irq, vcpu_id);

    }
    return is_spi_pending(gic_dist, irq);
}

static inline void set_sgi_ppi_enable(struct gic_dist_map *gic_dist, int irq, bool set_enable, int vcpu_id)
{
    if (set_enable) {
        gic_dist->enable_set0[vcpu_id] |= IRQ_BIT(irq);
        gic_dist->enable_clr0[vcpu_id] |= IRQ_BIT(irq);
    } else {
        gic_dist->enable_set0[vcpu_id] &= ~IRQ_BIT(irq);
        gic_dist->enable_clr0[vcpu_id] &= ~IRQ_BIT(irq);
    }
}

static inline void set_spi_enable(struct gic_dist_map *gic_dist, int irq, bool set_enable)
{
    if (set_enable) {
        gic_dist->enable_set[IRQ_IDX(irq)] |= IRQ_BIT(irq);
        gic_dist->enable_clr[IRQ_IDX(irq)] |= IRQ_BIT(irq);
    } else {
        gic_dist->enable_set[IRQ_IDX(irq)] &= ~IRQ_BIT(irq);
        gic_dist->enable_clr[IRQ_IDX(irq)] &= ~IRQ_BIT(irq);
    }
}

static inline void set_enable(struct gic_dist_map *gic_dist, int irq, bool set_enable, int vcpu_id)
{
    if (irq < NUM_VCPU_LOCAL_VIRQS) {
        set_sgi_ppi_enable(gic_dist, irq, set_enable, vcpu_id);
        return;
    }
    set_spi_enable(gic_dist, irq, set_enable);
}

static inline bool is_sgi_ppi_enabled(struct gic_dist_map *gic_dist, int irq, int vcpu_id)
{
    return !!(gic_dist->enable_set0[vcpu_id] & IRQ_BIT(irq));
}

static inline bool is_spi_enabled(struct gic_dist_map *gic_dist, int irq)
{
    return !!(gic_dist->enable_set[IRQ_IDX(irq)] & IRQ_BIT(irq));
}

static inline bool is_enabled(struct gic_dist_map *gic_dist, int irq, int vcpu_id)
{
    if (irq < NUM_VCPU_LOCAL_VIRQS) {
        return is_sgi_ppi_enabled(gic_dist, irq, vcpu_id);
    }
    return is_spi_enabled(gic_dist, irq);
}

static inline bool is_sgi_ppi_active(struct gic_dist_map *gic_dist, int irq, int vcpu_id)
{
    return !!(gic_dist->active0[vcpu_id] & IRQ_BIT(irq));
}

static inline bool is_spi_active(struct gic_dist_map *gic_dist, int irq)
{
    return !!(gic_dist->active[IRQ_IDX(irq)] & IRQ_BIT(irq));
}

static inline bool is_active(struct gic_dist_map *gic_dist, int irq, int vcpu_id)
{
    if (irq < NUM_VCPU_LOCAL_VIRQS) {
        return is_sgi_ppi_active(gic_dist, irq, vcpu_id);
    }
    return is_spi_active(gic_dist, irq);
}

static inline void vgic_dist_enable(struct gic_dist_map *dist)
{
    DDIST("Enabling distributor\n");
    dist->enable = 1;
}

static inline void vgic_dist_disable(struct gic_dist_map *dist)
{
    DDIST("Disabling distributor\n");
    dist->enable = 0;
}

static void vgic_dist_enable_irq(vgic_t *vgic, uint64_t vcpu_id, int irq)
{
    DDIST("Enabling IRQ %d\n", irq);
    set_enable(vgic->dist, irq, true, vcpu_id);
    struct virq_handle *virq_data = virq_find_irq_data(vgic, vcpu_id, irq);
    if (virq_data) {
        /* STATE b) */
        if (!is_pending(vgic->dist, virq_data->virq, vcpu_id)) {
            virq_ack(vcpu, virq_data);
        }
    } else {
        DDIST("Enabled IRQ %d has no handle\n", irq);
    }
}

static void vgic_dist_disable_irq(vgic_t *vgic, uint64_t vcpu_id, int irq)
{
    /* STATE g)
     *
     * It is IMPLEMENTATION DEFINED if a GIC allows disabling SGIs. Our vGIC
     * implementation does not allows it, such requests are simply ignored.
     * Since it is not uncommon that a guest OS tries disabling SGIs, e.g. as
     * part of the platform initialization, no dedicated messages are logged
     * here to avoid bloating the logs.
     */
    if (irq >= NUM_SGI_VIRQS) {
        DDIST("disabling irq %d\n", irq);
        set_enable(vgic->dist, irq, false, vcpu_id);
    }
}

static int vgic_dist_set_pending_irq(vgic_t *vgic, uint64_t vcpu_id, int irq)
{
    /* STATE c) */

    struct virq_handle *virq_data = virq_find_irq_data(vgic, vcpu_id, irq);

    if (!virq_data || !vgic->dist->enable || !is_enabled(vgic->dist, irq, vcpu_id)) {
        DDIST("IRQ not enabled (%d) on vcpu %d\n", irq, vcpu_id);
        return -1;
    }

    if (is_pending(vgic->dist, virq_data->virq, vcpu_id)) {
        return 0;
    }

    DDIST("Pending set: Inject IRQ from pending set (%d)\n", irq);
    set_pending(vgic->dist, virq_data->virq, true, vcpu_id);

    /* Enqueueing an IRQ and dequeueing it right after makes little sense
     * now, but in the future this is needed to support IRQ priorities.
     */
    int err = vgic_irq_enqueue(vgic, vcpu, virq_data);
    if (err) {
        ZF_LOGF("Failure enqueueing IRQ, increase MAX_IRQ_QUEUE_LEN");
        return -1;
    }

    int idx = vgic_find_empty_list_reg(vgic, vcpu);
    if (idx < 0) {
        /* There were no empty list registers available, but that's not a big
         * deal -- we have already enqueued this IRQ and eventually the vGIC
         * maintenance code will load it to a list register from the queue.
         */
        return 0;
    }

    struct virq_handle *virq = vgic_irq_dequeue(vgic, vcpu);
    halt(virq);

    return vgic_vcpu_load_list_reg(vgic, vcpu, idx, 0, virq);
}

static void vgic_dist_clr_pending_irq(struct gic_dist_map *dist, uint32_t vcpu_id, int irq)
{
    DDIST("Clear pending IRQ %d\n", irq);
    set_pending(dist, irq, false, vcpu_id);
    /* TODO: remove from IRQ queue and list registers as well */
    // @ivanv
}

static bool vgic_dist_reg_read(vm_t *vm, uint64_t vcpu_id,
                                                vgic_t *vgic, seL4_Word offset)
{
    int err = 0;
    fault_t *fault = vcpu->vcpu_arch.fault;
    halt(vgic->dist);
    struct gic_dist_map *gic_dist = vgic->dist;
    uint32_t reg = 0;
    int reg_offset = 0;
    uintptr_t base_reg;
    uint32_t *reg_ptr;
    switch (offset) {
    case RANGE32(GIC_DIST_CTLR, GIC_DIST_CTLR):
        reg = gic_dist->enable;
        break;
    case RANGE32(GIC_DIST_TYPER, GIC_DIST_TYPER):
        reg = gic_dist->ic_type;
        break;
    case RANGE32(GIC_DIST_IIDR, GIC_DIST_IIDR):
        reg = gic_dist->dist_ident;
        break;
    case RANGE32(0x00C, 0x01C):
        /* Reserved */
        break;
    case RANGE32(0x020, 0x03C):
        /* Implementation defined */
        break;
    case RANGE32(0x040, 0x07C):
        /* Reserved */
        break;
    case RANGE32(GIC_DIST_IGROUPR0, GIC_DIST_IGROUPR0):
        reg = gic_dist->irq_group0[vcpu_id];
        break;
    case RANGE32(GIC_DIST_IGROUPR1, GIC_DIST_IGROUPRN):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_IGROUPR1);
        reg = gic_dist->irq_group[reg_offset];
        break;
    case RANGE32(GIC_DIST_ISENABLER0, GIC_DIST_ISENABLER0):
        reg = gic_dist->enable_set0[vcpu_id];
        break;
    case RANGE32(GIC_DIST_ISENABLER1, GIC_DIST_ISENABLERN):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_ISENABLER1);
        reg = gic_dist->enable_set[reg_offset];
        break;
    case RANGE32(GIC_DIST_ICENABLER0, GIC_DIST_ICENABLER0):
        reg = gic_dist->enable_clr0[vcpu_id];
        break;
    case RANGE32(GIC_DIST_ICENABLER1, GIC_DIST_ICENABLERN):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_ICENABLER1);
        reg = gic_dist->enable_clr[reg_offset];
        break;
    case RANGE32(GIC_DIST_ISPENDR0, GIC_DIST_ISPENDR0):
        reg = gic_dist->pending_set0[vcpu_id];
        break;
    case RANGE32(GIC_DIST_ISPENDR1, GIC_DIST_ISPENDRN):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_ISPENDR1);
        reg = gic_dist->pending_set[reg_offset];
        break;
    case RANGE32(GIC_DIST_ICPENDR0, GIC_DIST_ICPENDR0):
        reg = gic_dist->pending_clr0[vcpu_id];
        break;
    case RANGE32(GIC_DIST_ICPENDR1, GIC_DIST_ICPENDRN):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_ICPENDR1);
        reg = gic_dist->pending_clr[reg_offset];
        break;
    case RANGE32(GIC_DIST_ISACTIVER0, GIC_DIST_ISACTIVER0):
        reg = gic_dist->active0[vcpu_id];
        break;
    case RANGE32(GIC_DIST_ISACTIVER1, GIC_DIST_ISACTIVERN):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_ISACTIVER1);
        reg = gic_dist->active[reg_offset];
        break;
    case RANGE32(GIC_DIST_ICACTIVER0, GIC_DIST_ICACTIVER0):
        reg = gic_dist->active_clr0[vcpu_id];
        break;
    case RANGE32(GIC_DIST_ICACTIVER1, GIC_DIST_ICACTIVERN):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_ICACTIVER1);
        reg = gic_dist->active_clr[reg_offset];
        break;
    case RANGE32(GIC_DIST_IPRIORITYR0, GIC_DIST_IPRIORITYR7):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_IPRIORITYR0);
        reg = gic_dist->priority0[vcpu_id][reg_offset];
        break;
    case RANGE32(GIC_DIST_IPRIORITYR8, GIC_DIST_IPRIORITYRN):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_IPRIORITYR8);
        reg = gic_dist->priority[reg_offset];
        break;
    case RANGE32(0x7FC, 0x7FC):
        /* Reserved */
        break;
    case RANGE32(GIC_DIST_ITARGETSR0, GIC_DIST_ITARGETSR7):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_ITARGETSR0);
        reg = gic_dist->targets0[vcpu_id][reg_offset];
        break;
    case RANGE32(GIC_DIST_ITARGETSR8, GIC_DIST_ITARGETSRN):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_ITARGETSR8);
        reg = gic_dist->targets[reg_offset];
        break;
    case RANGE32(0xBFC, 0xBFC):
        /* Reserved */
        break;
    case RANGE32(GIC_DIST_ICFGR0, GIC_DIST_ICFGRN):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_ICFGR0);
        reg = gic_dist->config[reg_offset];
        break;
    case RANGE32(0xD00, 0xDE4):
        base_reg = (uintptr_t) & (gic_dist->spi[0]);
        reg_ptr = (uint32_t *)(base_reg + (offset - 0xD00));
        reg = *reg_ptr;
        break;
    case RANGE32(0xDE8, 0xEFC):
        /* Reserved [0xDE8 - 0xE00) */
        /* GIC_DIST_NSACR [0xE00 - 0xF00) - Not supported */
        break;
    case RANGE32(GIC_DIST_SGIR, GIC_DIST_SGIR):
        reg = gic_dist->sgi_control;
        break;
    case RANGE32(0xF04, 0xF0C):
        /* Implementation defined */
        break;
    case RANGE32(GIC_DIST_CPENDSGIR0, GIC_DIST_CPENDSGIRN):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_CPENDSGIR0);
        reg = gic_dist->sgi_pending_clr[vcpu_id][reg_offset];
        break;
    case RANGE32(GIC_DIST_SPENDSGIR0, GIC_DIST_SPENDSGIRN):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_SPENDSGIR0);
        reg = gic_dist->sgi_pending_set[vcpu_id][reg_offset];
        break;
    case RANGE32(0xF30, 0xFBC):
        /* Reserved */
        break;
    case RANGE32(0xFC0, 0xFFB):
        base_reg = (uintptr_t) & (gic_dist->periph_id[0]);
        reg_ptr = (uint32_t *)(base_reg + (offset - 0xFC0));
        reg = *reg_ptr;
        break;
    default:
        printf("VMM|ERROR: Unknown register offset 0x%x", offset);
        err = ignore_fault(fault);
        goto fault_return;
    }
    uint32_t mask = fault_get_data_mask(fault);
    fault_set_data(fault, reg & mask);
    err = advance_fault(fault);

fault_return:
    // @ivanv: revisit, also make fault_return consistint. it's called something else in vgic_dist_reg_write
    if (err) {
        return false;
    }
    return true;
}

static inline void emulate_reg_write_access(uint32_t *vreg, fault_t *fault)
{
    *vreg = fault_emulate(fault, *vreg);
}

static bool vgic_dist_reg_write(vm_t *vm, uint64_t vcpu_id,
                                                 vgic_t *vgic, seL4_Word offset)
{
    int err = 0;
    fault_t *fault = vcpu->vcpu_arch.fault;
    halt(vgic->dist);
    struct gic_dist_map *gic_dist = vgic->dist;
    uint32_t reg = 0;
    uint32_t mask = fault_get_data_mask(fault);
    uint32_t reg_offset = 0;
    uint32_t data;
    switch (offset) {
    case RANGE32(GIC_DIST_CTLR, GIC_DIST_CTLR):
        data = fault_get_data(fault);
        if (data == 1) {
            vgic_dist_enable(gic_dist);
        } else if (data == 0) {
            vgic_dist_disable(gic_dist);
        } else {
            printf("VMM|ERROR: Unknown enable register encoding");
        }
        break;
    case RANGE32(GIC_DIST_TYPER, GIC_DIST_TYPER):
        // @ivanv: why break?
        break;
    case RANGE32(GIC_DIST_IIDR, GIC_DIST_IIDR):
        // @ivanv: why break?
        break;
    case RANGE32(0x00C, 0x01C):
        /* Reserved */
        break;
    case RANGE32(0x020, 0x03C):
        /* Implementation defined */
        break;
    case RANGE32(0x040, 0x07C):
        /* Reserved */
        break;
    case RANGE32(GIC_DIST_IGROUPR0, GIC_DIST_IGROUPR0):
        emulate_reg_write_access(&gic_dist->irq_group0[vcpu_id], fault);
        break;
    case RANGE32(GIC_DIST_IGROUPR1, GIC_DIST_IGROUPRN):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_IGROUPR1);
        emulate_reg_write_access(&gic_dist->irq_group[reg_offset], fault);
        break;
    case RANGE32(GIC_DIST_ISENABLER0, GIC_DIST_ISENABLERN):
        data = fault_get_data(fault);
        /* Mask the data to write */
        data &= mask;
        while (data) {
            int irq;
            irq = CTZ(data);
            data &= ~(1U << irq);
            irq += (offset - GIC_DIST_ISENABLER0) * 8;
            vgic_dist_enable_irq(vgic, vcpu, irq);
        }
        break;
    case RANGE32(GIC_DIST_ICENABLER0, GIC_DIST_ICENABLERN):
        data = fault_get_data(fault);
        /* Mask the data to write */
        data &= mask;
        while (data) {
            int irq;
            irq = CTZ(data);
            data &= ~(1U << irq);
            irq += (offset - GIC_DIST_ICENABLER0) * 8;
            vgic_dist_disable_irq(vgic, vcpu, irq);
        }
        break;
    case RANGE32(GIC_DIST_ISPENDR0, GIC_DIST_ISPENDRN):
        data = fault_get_data(fault);
        /* Mask the data to write */
        data &= mask;
        while (data) {
            int irq;
            irq = CTZ(data);
            data &= ~(1U << irq);
            irq += (offset - GIC_DIST_ISPENDR0) * 8;
            vgic_dist_set_pending_irq(vgic, vcpu, irq);
        }
        break;
    case RANGE32(GIC_DIST_ICPENDR0, GIC_DIST_ICPENDRN):
        data = fault_get_data(fault);
        /* Mask the data to write */
        data &= mask;
        while (data) {
            int irq;
            irq = CTZ(data);
            data &= ~(1U << irq);
            irq += (offset - GIC_DIST_ICPENDR0) * 8;
            vgic_dist_clr_pending_irq(vgic, vcpu, irq);
        }
        break;
    case RANGE32(GIC_DIST_ISACTIVER0, GIC_DIST_ISACTIVER0):
        emulate_reg_write_access(&gic_dist->active0[vcpu_id], fault);
        break;
    case RANGE32(GIC_DIST_ISACTIVER1, GIC_DIST_ISACTIVERN):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_ISACTIVER1);
        emulate_reg_write_access(&gic_dist->active[reg_offset], fault);
        break;
    case RANGE32(GIC_DIST_ICACTIVER0, GIC_DIST_ICACTIVER0):
        emulate_reg_write_access(&gic_dist->active_clr0[vcpu_id], fault);
        break;
    case RANGE32(GIC_DIST_ICACTIVER1, GIC_DIST_ICACTIVERN):
        reg_offset = GIC_DIST_REGN(offset, GIC_DIST_ICACTIVER1);
        emulate_reg_write_access(&gic_dist->active_clr[reg_offset], fault);
        break;
    case RANGE32(GIC_DIST_IPRIORITYR0, GIC_DIST_IPRIORITYRN):
        break;
    case RANGE32(0x7FC, 0x7FC):
        /* Reserved */
        break;
    case RANGE32(GIC_DIST_ITARGETSR0, GIC_DIST_ITARGETSRN):
        break;
    case RANGE32(0xBFC, 0xBFC):
        /* Reserved */
        break;
    case RANGE32(GIC_DIST_ICFGR0, GIC_DIST_ICFGRN):
        /* Not supported */
        break;
    case RANGE32(0xD00, 0xDE4):
        break;
    case RANGE32(0xDE8, 0xEFC):
        /* Reserved [0xDE8 - 0xE00) */
        /* GIC_DIST_NSACR [0xE00 - 0xF00) - Not supported */
        break;
    case RANGE32(GIC_DIST_SGIR, GIC_DIST_SGIR):
        data = fault_get_data(fault);
        int mode = (data & GIC_DIST_SGI_TARGET_LIST_FILTER_MASK) >> GIC_DIST_SGI_TARGET_LIST_FILTER_SHIFT;
        int virq = (data & GIC_DIST_SGI_INTID_MASK);
        uint16_t target_list = 0;
        switch (mode) {
        case GIC_DIST_SGI_TARGET_LIST_SPEC:
            /* Forward virq to vcpus specified in CPUTargetList */
            target_list = (data & GIC_DIST_SGI_CPU_TARGET_LIST_MASK) >> GIC_DIST_SGI_CPU_TARGET_LIST_SHIFT;
            break;
        case GIC_DIST_SGI_TARGET_LIST_OTHERS:
            /* Forward virq to all vcpus but the requesting vcpu */
            target_list = (1 << vcpu->vm->num_vcpus) - 1;
            target_list = target_list & ~(1 << vcpu_id);
            break;
        case GIC_DIST_SGI_TARGET_SELF:
            /* Forward to virq to only the requesting vcpu */
            target_list = (1 << vcpu_id);
            break;
        default:
            printf("VMM|ERROR: Unknow SGIR Target List Filter mode");
            goto ignore_fault;
        }
        // @ivanv: Here we're making the assumption that there's only one vCPU, and
        // we're also blindly injectnig the given IRQ to that vCPU.
        vm_inject_irq(0, virq);
        break;
    case RANGE32(0xF04, 0xF0C):
        break;
    case RANGE32(GIC_DIST_CPENDSGIR0, GIC_DIST_SPENDSGIRN):
        halt(!"vgic SGI reg not implemented!\n");
        break;
    case RANGE32(0xF30, 0xFBC):
        /* Reserved */
        break;
    case RANGE32(0xFC0, 0xFFB):
        break;
    default:
        printf("VMM|ERROR: Unknown register offset 0x%x", offset);
    }
ignore_fault:
    // @ivanv: revisit
    err = ignore_fault(fault);
    if (err) {
        return false;
    }
    return true;
}

bool handle_vgic_dist_fault(uint64_t vcpu_id, uintptr_t fault_addr)
{
    // @ivanv: revisit this comment
    /* There is a fault object per VCPU with much more context, the parameters
     * fault_addr and fault_length are no longer used.
     */
    fault_t *fault = vcpu->vcpu_arch.fault;
    halt(fault);
    halt(fault_addr == fault_get_address(vcpu->vcpu_arch.fault));

    halt(cookie);
    struct vgic_dist_device *d = (typeof(d))cookie;
    vgic_t *vgic = d->vgic;
    halt(vgic->dist);

    seL4_Word addr = fault_get_address(fault);
    halt(addr >= d->pstart);
    seL4_Word offset = addr - d->pstart;
    halt(offset < PAGE_SIZE_4K);

    return fault_is_read(fault) ? vgic_dist_reg_read(vm, vcpu, vgic, offset)
           : vgic_dist_reg_write(vm, vcpu, vgic, offset);
}
