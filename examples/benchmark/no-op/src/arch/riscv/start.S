#define STACK_SIZE  0x4000

.section .start, "ax"
.global _start
_start:
    /**
     * Expecting hart id in a0.
     * Save hart id in thread pointer. We assume the compiler does not use tp.
     */
    mv      tp, a0 
.option push
.option norelax   
    /* Initialize global_pointer */
	la gp, __global_pointer$
.option pop

.pushsection .data
.align 3
.global primary_hart
primary_hart: .word 0x0
primary_hart_valid: .word 0x0
.popsection


    la  t0, primary_hart_valid
    li  t1, 1
1:
    lr.w  t2, (t0)
    bnez  t2, skip
    sc.w  t2, t1, (t0)
    bnez  t2, 1b
    la    t0, primary_hart
    sw    a0, 0(t0)

    /* Clear bss */ 
    la t0, __bss_start
    la t1, __bss_end
    bgeu t0, t1, 2f
1:
    sw zero, (t0)
    addi t0, t0, 4
    bltu t0, t1, 1b
2:

.pushsection .data
.align 3
primary_hart_ready: .word 0x0
.popsection

    la      t0, primary_hart_ready
    li      t1, 1
    sw      t1, 0(t0)

skip:

    la      t0, exception_handler
    csrw    stvec, t0

    la      t0, primary_hart_ready
1:
    lw      t1, 0(t0)
    beqz    t1, 1b

    /* Initialize stack pointer*/
    la      t0, _stack_base
    li      t1, STACK_SIZE
    add     t0, t0, t1
#ifndef SINGLE_CORE
    mul     t1, t1, tp
    add     t0, t0, t1
#endif
    mv      sp, t0

    //TODO: other c runtime init (ctors, etc...)
    
    /* Jump to main */
    call _init
    j .

.global _fini
_fini: ret
