#ifndef __PMU_H__
#define __PMU_H__

#include <stdint.h>
#include <stdbool.h>

#include <bit.h>
#include <sysregs.h>
#include <fences.h>

#define PMU_MAX_NUM_COUNTERS    (32)

#define PMCR_N_OFF (11)
#define PMCR_N_LEN (5)

#define NO_EL10 (0x3 << 30)
#define WITH_EL2 (1 << 27)
#define EL2_ONLY (NO_EL10 | WITH_EL2)

enum PMU_EVENT {
    SW_INCR = 0x0, 
    L1I_CACHE_REFILL,
    L1I_TLB_REFILL,
    L1D_CACHE_REFILL,
    L1D_CACHE,
    L1D_TLB_REFILL,
    LD_RETIRED,
    ST_RETIRED,
    INST_RETIRED,
    EXC_TAKEN,
    EXC_RETURN,
    CID_WRITE_RETIRED,
    PC_WRITE_RETIRED,
    BR_IMMED_RETIRED,
    BR_RETURN_RETIRED,
    UNALIGNED_LDST_RETIRED,
    BR_MIS_PRED,
    CPU_CYCLES,
    BR_PRED,
    MEM_ACCESS,
    L1I_CACHE,
    L1D_CACHE_WB,
    L2D_CACHE,
    L2D_CACHE_REFILL,
    L2D_CACHE_WB,
    BUS_ACCESS,
    MEMORY_ERROR,
    INST_SPEC,
    TTBR_WRITE_RETIRED,
    BUS_CYCLES,
    CHAIN,
    L1D_CACHE_ALLOCATE,
    L2D_CACHE_ALLOCATE,
    BR_RETIRED,
    BR_MIS_PRED_RETIRED,
    STALL_FRONTEND,
    STALL_BACKEND,
    L1D_TLB,
    L1I_TLB,
    L2I_CACHE,
    L2I_CACHE_REFILL,
    L3D_CACHE_ALLOCATE,
    L3D_CACHE_REFILL,
    L3D_CACHE,
    L3D_CACHE_WB,
    L2D_TLB_REFILL,
    L2I_TLB_REFILL,
    L2D_TLB,
    L2I_TLB,
    REMOTE_ACCESS,
    LL_CACHE,
    LL_CACHE_MISS,
    DTLB_WALKa,
    ITLB_WALKa,
    LL_CACHE_RD,
    LL_CACHE_MISS_RD,
    REMOTE_ACCESS_RD,
    L1D_CACHE_LMISS_RD,
    OP_RETIRED,
    OP_SPEC,
    STALL,
    STALL_SLOT_BACKEND,
    STALL_SLOT_FRONTEND,
    STALL_SLOT,

    SAMPLE_POP = 0x4000,
    SAMPLE_FEED,
    SAMPLE_FILTRATE,
    SAMPLE_COLLISION,
    CNT_CYCLES,
    STALL_BACKEND_MEM,
    L1I_CACHE_LMISS,
    L2D_CACHE_LMISS_RD = 0x4009,
    L2I_CACHE_LMISS,
    L3D_CACHE_LMISS_RD,
    SVE_INST_RETIRED = 0x8002,
    SVE_INST_SPEC = 0x8006,
    /* Recommended implementation defined */
    L1D_CACHE_RD = 0x40,
    L1D_CACHE_WR,
    L1D_CACHE_REFILL_RD,
    L1D_CACHE_REFILL_WR,
    L1D_CACHE_REFILL_INNE,
    L1D_CACHE_REFILL_OUT,
    L1D_CACHE_WB_VICTIM,
    L1D_CACHE_WB_CLEAN,
    L1D_CACHE_INVAL,
    L1D_TLB_REFILL_RD = 0x4c,
    L1D_TLB_REFILL_WR,
    L1D_TLB_RD,
    L1D_TLB_WR,
    L2D_CACHE_RD,
    L2D_CACHE_WR,
    L2D_CACHE_REFILL_RD,
    L2D_CACHE_REFILL_WR,
    L2D_CACHE_WB_VICTIM = 0x56,
    L2D_CACHE_WB_CLEAN,
    L2D_CACHE_INVAL,
    L2D_TLB_REFILL_RD = 0x5c,
    L2D_TLB_REFILL_WR,
    L2D_TLB_RD,
    L2D_TLB_WR,
    BUS_ACCESS_RD,
    BUS_ACCESS_WR,
    BUS_ACCESS_SHARED,
    BUS_ACCESS_NOT_SHARED,
    BUS_ACCESS_NORMAL,
    BUS_ACCESS_PERIPH,
    MEM_ACCESS_RD,
    MEM_ACCESS_WR,
    UNALIGNED_LD_SPEC,
    UNALIGNED_ST_SPEC,
    UNALIGNED_LDST_SPEC,
    LDREX_SPEC = 0x6c,
    STREX_PASS_SPEC,
    STREX_FAIL_SPEC,
    STREX_SPEC,
    LD_SPEC,
    ST_SPEC,
    LDST_SPEC,
    DP_SPEC,
    ASE_SPEC,
    VFP_SPEC,
    PC_WRITE_SPEC,
    CRYPTO_SPEC,
    BR_IMMED_SPEC,
    BR_RETURN_SPEC,
    BR_INDIRECT_SPEC,
    ISB_SPEC = 0x7c,
    DSB_SPEC,
    DMB_SPEC,
    EXC_UNDEF = 0x81,
    EXC_SVC,
    EXC_PABORT,
    EXC_DABORT,
    EXC_IRQ = 0x86,
    EXC_FIQ,
    EXC_SMC,
    EXC_HVC = 0x8a,
    EXC_TRAP_PABORT,
    EXC_TRAP_DABORT,
    EXC_TRAP_OTHER,
    EXC_TRAP_IRQ,
    EXC_TRAP_FIQ,
    RC_LD_SPEC,
    RC_ST_SPEC,
    L3D_CACHE_RD = 0xa0,
    L3D_CACHE_WR,
    L3D_CACHE_REFILL_RD,
    L3D_CACHE_REFILL_WR,
    L3D_CACHE_WB_VICTIM = 0xa6,
    L3D_CACHE_WB_CLEAN,
    L3D_CACHE_INVAL,


    /* Cortex A-53 */
    LINEFILL_PREFETCH = 0xc2,
};

char const * const pmu_event_descr[] = {
    [L1D_CACHE_REFILL] =    "l1$d_miss",
    [L1I_CACHE_REFILL] =    "l1$i_miss",
    [L1D_CACHE] =           "l1$d_acc",
    [L2D_CACHE_REFILL] =    "l2$d_miss",
    [L2D_CACHE] =           "l2$d_accs",
    [L1D_CACHE_WB] =        "l1$d_wb",
    [L2D_CACHE_WB] =        "l2$d_wbs",
    [L1D_CACHE_ALLOCATE] =  "l1$d_all",
    [L2D_CACHE_ALLOCATE] =  "l2$d_all",
    [LL_CACHE] =            "llc_accs",
    [LL_CACHE_MISS] =       "llc_miss",
    [LINEFILL_PREFETCH] =   "pref_fill",
    [MEM_ACCESS] =          "mem_acc",
    [BUS_ACCESS] =          "bus_acc",
    [EXC_TAKEN] =           "exceptions",
    [EXC_IRQ] =             "irqs",
    [EXC_FIQ] =             "fiqs",
    [L1I_TLB_REFILL] =      "tlb_il1_miss",
    [L1D_TLB_REFILL] =      "tlb_dl1_miss",
    [INST_RETIRED]   =      "inst_ret",
};

static inline void pmu_start(){
    MSR(PMCR_EL0, 0x1);
}

static inline void pmu_stop(){
    MSR(PMCR_EL0, 0x0);
}

static inline void pmu_reset(){
    unsigned long pmcr = MRS(PMCR_EL0);
    MSR(PMCR_EL0, pmcr | 0x6);
}

static inline size_t pmu_num_counters() {
    return (size_t) bit_extract(MRS(PMCR_EL0), PMCR_N_OFF, PMCR_N_LEN);
}

static inline void pmu_counter_set_event(size_t counter, size_t event){
    MSR(PMSELR_EL0, counter);
    fence_sync_write();
    MSR(PMXEVTYPER_EL0, event);
}

static inline size_t pmu_counter_get_event(size_t counter){
    MSR(PMSELR_EL0, counter);
    fence_sync_write();
    return MRS(PMXEVTYPER_EL0);
}

static inline unsigned long pmu_counter_get(size_t counter){
    MSR(PMSELR_EL0, counter);
    //barrier?
    return MRS(PMXEVCNTR_EL0);
}

static inline void pmu_counter_set(size_t counter, unsigned long value){
    MSR(PMSELR_EL0, counter);
    //barrier?
    MSR(PMXEVCNTR_EL0, value);
}

static inline void pmu_counter_enable(size_t counter) {
    MSR(PMCNTENSET_EL0, 1UL << counter);
}

static inline void pmu_counter_disable(size_t counter) {
    MSR(PMCNTENCLR_EL0, 1UL << counter);
}

static inline void pmu_cycle_enable(bool en){
    uint64_t val = (1ULL << 31);
    if(en){
        MSR(PMCNTENSET_EL0, val);
    } else {
        MSR(PMCNTENCLR_EL0, val);
    }
}

static inline uint64_t pmu_cycle_get(){
    uint64_t val = 0;
    return MRS(PMCCNTR_EL0);
}

#endif /* __PMU_H__ */
