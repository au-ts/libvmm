/** 
 * baohu separation kernel
 *
 * Copyright (c) Jose Martins, Sandro Pinto
 *
 * Authors:
 *      Jose Martins <josemartins90@gmail.com>
 *
 * baohu is free software; you can redistribute it and/or modify it under the
 * terms of the GNU General Public License version 2 as published by the Free
 * Software Foundation, with a special exception exempting guest code from such
 * license. See the COPYING file in the top-level directory for details. 
 *
 */

#ifndef __ARCH_CSR_H__
#define __ARCH_CSR_H__

#define RV64   (__riscv_xlen == 64)
#define RV32   (__riscv_xlen == 32)

#if(!(RV64))
    #error "Unsupported __riscv_xlen #__riscv_xlen."
#endif

#if(RV64)
    #define LOAD    ld
    #define STORE   sd
    #define REGLEN  (8)
#elif(RV32)
    #define LOAD    lw
    #define STORE   sw
    #define REGLEN  (4)
#endif

#if(RV64)
    #define BAO_VAS_BASE    (0xffffffc000000000)
    #define BAO_CPU_BASE    (0xffffffc040000000)
    #define BAO_VM_BASE     (0xffffffe000000000)
    #define BAO_VAS_TOP     (0xfffffff000000000)
#elif(RV32)
    #define BAO_VAS_BASE    (0xc0000000)
    #define BAO_CPU_BASE    (0x00000000)
    #define BAO_VM_BASE     (0x00000000)
    #define BAO_VAS_TOP     (0xffffffff)
#endif

#define REG_RA  (1)
#define REG_SP  (2)
#define REG_GP  (3)
#define REG_TP  (4)
#define REG_T0  (5)
#define REG_T1  (6)
#define REG_T2  (7)
#define REG_S0  (8)
#define REG_S1  (9)
#define REG_A0  (10)
#define REG_A1  (11)
#define REG_A2  (12)
#define REG_A3  (13)
#define REG_A4  (14)
#define REG_A5  (15)
#define REG_A6  (16)
#define REG_A7  (17)
#define REG_S2  (18)
#define REG_S3  (19)
#define REG_S4  (20)
#define REG_S5  (21)
#define REG_S6  (22)
#define REG_S7  (23)
#define REG_S8  (24)
#define REG_S9  (25)
#define REG_S10 (26) 
#define REG_S11 (27) 
#define REG_T3  (28) 
#define REG_T4  (29) 
#define REG_T5  (30) 
#define REG_T6  (31) 

#define PRIV_U  (0)
#define PRIV_S  (1)
#define PRIV_M  (3)

#define CSR_STR(s)      #s

#define STVEC_MODE_OFF      (0)
#define STVEC_MODE_LEN      (2)
#define STVEC_MODE_MSK      BIT_MASK(STVEC_MODE_OFF, STVEC_MODE_LEN)
#define STVEC_MODE_DIRECT   (0)
#define STVEC_MODE_VECTRD   (1)

#if(RV64)
    #define SATP_MODE_OFF   (60)
    #define SATP_MODE_DFLT  SATP_MODE_39
    #define SATP_ASID_OFF   (44)
    #define SATP_ASID_LEN   (16)
#elif(RV32)
    #define SATP_MODE_OFF   (31)
    #define SATP_MODE_DFLT  SATP_MODE_32
    #define SATP_ASID_OFF   (22)
    #define SATP_ASID_LEN   (9)
#endif

#define SATP_MODE_BARE  (0ULL << SATP_MODE_OFF)
#define SATP_MODE_32    (1ULL << SATP_MODE_OFF)
#define SATP_MODE_39    (8ULL << SATP_MODE_OFF)
#define SATP_MODE_48    (9ULL << SATP_MODE_OFF)
#define SATP_ASID_MSK   BIT_MASK(SATP_ASID_OFF,SATP_ASID_LEN)

#define SSTATUS_UIE     (1ULL << 0)
#define SSTATUS_SIE     (1ULL << 1)
#define SSTATUS_UPIE    (1ULL << 4)
#define SSTATUS_SPIE    (1ULL << 5)
#define SSTATUS_SPP     (1ULL << 8)

#define SIE_USIE    (1ULL << 0)
#define SIE_SSIE    (1ULL << 1)
#define SIE_UTIE    (1ULL << 4)
#define SIE_STIE    (1ULL << 5)
#define SIE_UEIE    (1ULL << 8)
#define SIE_SEIE    (1ULL << 9)    

#define SIP_USIE    SIE_USIE
#define SIP_SSIE    SIE_SSIE
#define SIP_UTIE    SIE_UTIE
#define SIP_STIE    SIE_STIE
#define SIP_UEIE    SIE_UEIE
#define SIP_SEIE    SIE_SEIE    

#define SCAUSE_INT_BIT      (1ULL << ((REGLEN*8)-1))
#define SCAUSE_CODE_MSK     (SCAUSE_INT_BIT-1)
#define SCAUSE_CODE_USI     (0 | SCAUSE_INT_BIT)     
#define SCAUSE_CODE_SSI     (1 | SCAUSE_INT_BIT)
#define SCAUSE_CODE_UTI     (4 | SCAUSE_INT_BIT)
#define SCAUSE_CODE_STI     (5 | SCAUSE_INT_BIT)
#define SCAUSE_CODE_UEI     (8 | SCAUSE_INT_BIT)
#define SCAUSE_CODE_SEI     (9 | SCAUSE_INT_BIT)
#define SCAUSE_CODE_IAM     (0)
#define SCAUSE_CODE_IAF     (1)
#define SCAUSE_CODE_ILI     (2)
#define SCAUSE_CODE_BKP     (3)
#define SCAUSE_CODE_LAM     (4)
#define SCAUSE_CODE_LAF     (5)
#define SCAUSE_CODE_SAM     (6)
#define SCAUSE_CODE_SAF     (7)
#define SCAUSE_CODE_ECU     (8)
#define SCAUSE_CODE_ECS     (9)
#define SCAUSE_CODE_ECV     (10)
#define SCAUSE_CODE_IPF     (12)
#define SCAUSE_CODE_LPF     (13)
#define SCAUSE_CODE_SPF     (15)

#define CSR_STR(s) #s

#define CSRR(csr)                                     \
    ({                                                \
        uint64_t _temp;                               \
        asm volatile("csrr  %0, " CSR_STR(csr) "\n\r" \
                     : "=r"(_temp)::"memory");        \
        _temp;                                        \
    })

#define CSRW(csr, rs) \
    asm volatile("csrw  " CSR_STR(csr) ", %0\n\r" ::"rK"(rs) : "memory")
#define CSRS(csr, rs) \
    asm volatile("csrs  " CSR_STR(csr) ", %0\n\r" ::"rK"(rs) : "memory")
#define CSRC(csr, rs) \
    asm volatile("csrc  " CSR_STR(csr) ", %0\n\r" ::"rK"(rs) : "memory")


#endif /* __ARCH_CSRS_H__ */
