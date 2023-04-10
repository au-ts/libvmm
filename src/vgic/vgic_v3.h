/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2019, DornerWorks
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#define GIC_500_GRP0     (1 << 0)
#define GIC_500_GRP1_NS  (1 << 1)
#define GIC_500_GRP1_S   (1 << 2)
#define GIC_500_ARE_S    (1 << 4)

#define GIC_500_ENABLED (GIC_500_ARE_S | GIC_500_GRP1_NS | GIC_500_GRP0)
#define GIC_ENABLED GIC_500_ENABLED

#define GIC_SGI_OFFSET  0x10000
#define GIC_MAX_DISABLE 32
#define GIC_GROUP       1

struct gic_dist_map {
    uint32_t ctlr;                /* 0x0000 */
    uint32_t typer;               /* 0x0004 */
    uint32_t iidr;                /* 0x0008 */
    uint32_t res1[13];            /* [0x000C, 0x0040) */
    uint32_t setspi_nsr;          /* 0x0040 */
    uint32_t res2;                /* 0x0044 */
    uint32_t clrspi_nsr;          /* 0x0048 */
    uint32_t res3;                /* 0x004C */
    uint32_t setspi_sr;           /* 0x0050 */
    uint32_t res4;                /* 0x0054 */
    uint32_t clrspi_sr;           /* 0x0058 */
    uint32_t res5[9];             /* [0x005C, 0x0080) */
    uint32_t irq_group0[GUEST_NUM_VCPUS];          /* [0x080, 0x84) */
    uint32_t irq_group[31];                             /* [0x084, 0x100) */
    uint32_t enable_set0[GUEST_NUM_VCPUS];         /* [0x100, 0x104) */
    uint32_t enable_set[31];                            /* [0x104, 0x180) */
    uint32_t enable_clr0[GUEST_NUM_VCPUS];         /* [0x180, 0x184) */
    uint32_t enable_clr[31];                            /* [0x184, 0x200) */
    uint32_t pending_set0[GUEST_NUM_VCPUS];        /* [0x200, 0x204) */
    uint32_t pending_set[31];                           /* [0x204, 0x280) */
    uint32_t pending_clr0[GUEST_NUM_VCPUS];        /* [0x280, 0x284) */
    uint32_t pending_clr[31];                           /* [0x284, 0x300) */
    uint32_t active0[GUEST_NUM_VCPUS];             /* [0x300, 0x304) */
    uint32_t active[31];                                /* [0x300, 0x380) */
    uint32_t active_clr0[GUEST_NUM_VCPUS];         /* [0x380, 0x384) */
    uint32_t active_clr[31];                            /* [0x384, 0x400) */
    uint32_t priority0[GUEST_NUM_VCPUS][8];        /* [0x400, 0x420) */
    uint32_t priority[247];                             /* [0x420, 0x7FC) */
    uint32_t res6;                  /* 0x7FC */

    uint32_t targets0[GUEST_NUM_VCPUS][8];         /* [0x800, 0x820) */
    uint32_t targets[247];                              /* [0x820, 0xBFC) */
    uint32_t res7;                  /* 0xBFC */

    uint32_t config[64];            /* [0xC00, 0xD00) */
    uint32_t group_mod[64];         /* [0xD00, 0xE00) */
    uint32_t nsacr[64];             /* [0xE00, 0xF00) */
    uint32_t sgir;                  /* 0xF00 */
    uint32_t res8[3];               /* [0xF00, 0xF10) */
    uint32_t sgi_pending_clr[GUEST_NUM_VCPUS][4];  /* [0xF10, 0xF20) */
    uint32_t sgi_pending_set[GUEST_NUM_VCPUS][4];  /* [0xF20, 0xF30) */
    uint32_t res9[5235];            /* [0x0F30, 0x6100) */

    uint64_t irouter[960];          /* [0x6100, 0x7F00) */
    uint64_t res10[2080];           /* [0x7F00, 0xC000) */
    uint32_t estatusr;              /* 0xC000 */
    uint32_t errtestr;              /* 0xC004 */
    uint32_t res11[31];             /* [0xC008, 0xC084) */
    uint32_t spisr[30];             /* [0xC084, 0xC0FC) */
    uint32_t res12[4021];           /* [0xC0FC, 0xFFD0) */

    uint32_t pidrn[8];              /* [0xFFD0, 0xFFF0) */
    uint32_t cidrn[4];              /* [0xFFD0, 0xFFFC] */
};

/* Memory map for GIC Redistributor Registers for control and physical LPI's */
struct gic_redist_map {          /* Starting */
    uint32_t    ctlr;           /* 0x0000 */
    uint32_t    iidr;           /* 0x0004 */
    uint64_t    typer;          /* 0x008 */
    uint32_t    res0;           /* 0x0010 */
    uint32_t    waker;          /* 0x0014 */
    uint32_t    res1[21];       /* 0x0018 */
    uint64_t    propbaser;      /* 0x0070 */
    uint64_t    pendbaser;      /* 0x0078 */
    uint32_t    res2[16340];    /* 0x0080 */
    uint32_t    pidr4;          /* 0xFFD0 */
    uint32_t    pidr5;          /* 0xFFD4 */
    uint32_t    pidr6;          /* 0xFFD8 */
    uint32_t    pidr7;          /* 0xFFDC */
    uint32_t    pidr0;          /* 0xFFE0 */
    uint32_t    pidr1;          /* 0xFFE4 */
    uint32_t    pidr2;          /* 0xFFE8 */
    uint32_t    pidr3;          /* 0xFFEC */
    uint32_t    cidr0;          /* 0xFFF0 */
    uint32_t    cidr1;          /* 0xFFF4 */
    uint32_t    cidr2;          /* 0xFFF8 */
    uint32_t    cidr3;          /* 0xFFFC */
};

/* Memory map for the GIC Redistributor Registers for the SGI and PPI's */
// struct gic_redist_sgi_ppi_map {  /* Starting */
//     uint32_t    res0[32];       /* 0x0000 */
//     uint32_t    igroup[32];     /* 0x0080 */
//     uint32_t    isenable[32];   /* 0x0100 */
//     uint32_t    icenable[32];   /* 0x0180 */
//     uint32_t    ispend[32];     /* 0x0200 */
//     uint32_t    icpend[32];     /* 0x0280 */
//     uint32_t    isactive[32];   /* 0x0300 */
//     uint32_t    icactive[32];   /* 0x0380 */
//     uint32_t    ipriorityrn[8]; /* 0x0400 */
//     uint32_t    res1[504];      /* 0x0420 */
//     uint32_t    icfgrn_ro;      /* 0x0C00 */
//     uint32_t    icfgrn_rw;      /* 0x0C04 */
//     uint32_t    res2[62];       /* 0x0C08 */
//     uint32_t    igrpmod[64];    /* 0x0D00 */
//     uint32_t    nsac;           /* 0x0E00 */
//     uint32_t    res11[11391];   /* 0x0E04 */
//     uint32_t    miscstatsr;     /* 0xC000 */
//     uint32_t    res3[31];       /* 0xC004 */
//     uint32_t    ppisr;          /* 0xC080 */
//     uint32_t    res4[4062];     /* 0xC084 */
// };

/*
 * GIC Distributor Register Map
 * ARM Generic Interrupt Controller (Architecture version 3.0)
 * Architecture Specification (Issue C)
 * Chapter 8 Programmers' Model - Table 8-25
 */
#define GIC_DIST_CTLR           0x000
#define GIC_DIST_TYPER          0x004
#define GIC_DIST_IIDR           0x008
#define GIC_DIST_STATUSR        0x010
#define GIC_DIST_SETSPI_NSR     0x040
#define GIC_DIST_CLRSPI_NSR     0x048
#define GIC_DIST_SETSPI_SR      0x050
#define GIC_DIST_CLRSPI_SR      0x058
#define GIC_DIST_IGROUPR0       0x080
#define GIC_DIST_IGROUPR1       0x084
#define GIC_DIST_IGROUPRN       0x0FC
#define GIC_DIST_ISENABLER0     0x100
#define GIC_DIST_ISENABLER1     0x104
#define GIC_DIST_ISENABLERN     0x17C
#define GIC_DIST_ICENABLER0     0x180
#define GIC_DIST_ICENABLER1     0x184
#define GIC_DIST_ICENABLERN     0x1fC
#define GIC_DIST_ISPENDR0       0x200
#define GIC_DIST_ISPENDR1       0x204
#define GIC_DIST_ISPENDRN       0x27C
#define GIC_DIST_ICPENDR0       0x280
#define GIC_DIST_ICPENDR1       0x284
#define GIC_DIST_ICPENDRN       0x2FC
#define GIC_DIST_ISACTIVER0     0x300
#define GIC_DIST_ISACTIVER1     0x304
#define GIC_DIST_ISACTIVERN     0x37C
#define GIC_DIST_ICACTIVER0     0x380
#define GIC_DIST_ICACTIVER1     0x384
#define GIC_DIST_ICACTIVERN     0x3FC
#define GIC_DIST_IPRIORITYR0    0x400
#define GIC_DIST_IPRIORITYR7    0x41C
#define GIC_DIST_IPRIORITYR8    0x420
#define GIC_DIST_IPRIORITYRN    0x7F8
#define GIC_DIST_ITARGETSR0     0x800
#define GIC_DIST_ITARGETSR7     0x81C
#define GIC_DIST_ITARGETSR8     0x820
#define GIC_DIST_ITARGETSRN     0xBF8
#define GIC_DIST_ICFGR0         0xC00
#define GIC_DIST_ICFGRN         0xCFC
#define GIC_DIST_IGRPMODR0      0xD00
#define GIC_DIST_IGRPMODRN      0xD7C
#define GIC_DIST_NSACR0         0xE00
#define GIC_DIST_NSACRN         0xEFC
#define GIC_DIST_SGIR           0xF00
#define GIC_DIST_CPENDSGIR0     0xF10
#define GIC_DIST_CPENDSGIRN     0xF1C
#define GIC_DIST_SPENDSGIR0     0xF20
#define GIC_DIST_SPENDSGIRN     0xF2C
#define GIC_DIST_IROUTER0      0x6100
#define GIC_DIST_IROUTERN      0x7FD8

/*
 * ARM Generic Interrupt Controller (Architecture version 3.0)
 * Architecture Specification (Issue C)
 * 8.9.21: Software Generated Interrupt Register, GICD_SGIR
 */
#define GIC_DIST_SGI_TARGET_LIST_FILTER_SHIFT   24
#define GIC_DIST_SGI_TARGET_LIST_FILTER_MASK    0x3 << GIC_DIST_SGI_TARGET_LIST_FILTER_SHIFT
#define GIC_DIST_SGI_TARGET_LIST_SPEC           0
#define GIC_DIST_SGI_TARGET_LIST_OTHERS         1
#define GIC_DIST_SGI_TARGET_SELF                2

#define GIC_DIST_SGI_CPU_TARGET_LIST_SHIFT      16
#define GIC_DIST_SGI_CPU_TARGET_LIST_MASK       0xFF << GIC_DIST_SGI_CPU_TARGET_LIST_SHIFT

#define GIC_DIST_SGI_INTID_MASK                 0xF

#define GICR_CTLR           0x000
#define GICR_IIDR           0x004
#define GICR_TYPER          0x008
#define GICR_WAKER          0x014

#define GICR_IGROUPR0       0x10080
#define GICR_ISENABLER0     0x10100
#define GICR_ICENABLER0     0x10180
#define GICR_ICACTIVER0     0x10380
#define GICR_IPRIORITYR0    0x10400
#define GICR_IPRIORITYRN    0x1041C
#define GICR_ICFGR1         0x10c04

typedef struct {
    /// Virtual distributor registers
    struct gic_dist_map *dist;
    /// Virtual redistributor registers for control and physical LPIs
    struct gic_redist_map *redist;
    /// Virtual redistributor for SGI and PPIs
    // struct gic_redist_sgi_ppi_map *sgi;
} vgic_reg_t;

static inline bool vgic_dist_is_enabled(struct gic_dist_map *gic_dist) {
    return gic_dist->ctlr & GIC_500_GRP1_NS;
}

static inline void vgic_dist_enable(struct gic_dist_map *gic_dist) {
    gic_dist->ctlr |= GIC_500_GRP1_NS | GIC_500_ARE_S;
}

static inline void vgic_dist_disable(struct gic_dist_map *gic_dist)
{
    gic_dist->ctlr &= ~(GIC_500_GRP1_NS | GIC_500_ARE_S);
}

static inline struct gic_dist_map *vgic_get_dist(void *registers)
{
    assert(registers);
    return ((vgic_reg_t *) registers)->dist;
}

static inline struct gic_redist_map *vgic_get_redist(void *registers)
{
    assert(registers);
    return ((vgic_reg_t *) registers)->redist;
}

