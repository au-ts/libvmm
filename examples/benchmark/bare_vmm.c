/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <microkit.h>
#include "util/util.h"
#include "arch/aarch64/vgic/vgic.h"
#include "arch/aarch64/linux.h"
#include "arch/aarch64/fault.h"
#include "guest.h"
#include "virq.h"
#include "tcb.h"
#include "vcpu.h"
#include "sel4bench.h"
#include "temp_bench.h"
#include "bench.h"

#define START_PMU 2
#define STOP_PMU 3

// @ivanv: ideally we would have none of these hardcoded values
// initrd, ram size come from the DTB
// We can probably add a node for the DTB addr and then use that.
// Part of the problem is that we might need multiple DTBs for the same example
// e.g one DTB for VMM one, one DTB for VMM two. we should be able to hide all
// of this in the build system to avoid doing any run-time DTB stuff.

/*
 * As this is just an example, for simplicity we just make the size of the
 * guest's "RAM" the same for all platforms. For just booting Linux with a
 * simple user-space, 0x10000000 bytes (256MB) is plenty.
 */
#define GUEST_RAM_SIZE 0x10000000

#if defined(BOARD_qemu_arm_virt)
#define GUEST_DTB_VADDR 0x4f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x4d700000
#elif defined(BOARD_rpi4b_hyp)
#define GUEST_DTB_VADDR 0x2e000000
#define GUEST_INIT_RAM_DISK_VADDR 0x2d700000
#elif defined(BOARD_odroidc2_hyp)
#define GUEST_DTB_VADDR 0x2f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x2d700000
#elif defined(BOARD_odroidc4)
#define GUEST_DTB_VADDR 0x2f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x2d700000
#elif defined(BOARD_imx8mm_evk_hyp)
#define GUEST_DTB_VADDR 0x4f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x4d700000
#elif defined(BOARD_zcu102)
#define GUEST_DTB_VADDR 0x4f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x4d700000
#elif defined(BOARD_imx8mq_evk) || defined(BOARD_maaxboard)
#define GUEST_DTB_VADDR 0x4f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x4c000000
#else
#error Need to define guest kernel image address and DTB address
#endif

/* For simplicity we just enforce the serial IRQ channel number to be the same
 * across platforms. */
#define SERIAL_IRQ_CH 1

#if defined(BOARD_qemu_arm_virt)
#define SERIAL_IRQ 33
#elif defined(BOARD_odroidc2_hyp) || defined(BOARD_odroidc4)
#define SERIAL_IRQ 225
#elif defined(BOARD_rpi4b_hyp)
#define SERIAL_IRQ 57
#elif defined(BOARD_imx8mm_evk_hyp)
#define SERIAL_IRQ 79
#elif defined(BOARD_zcu102)
#define SERIAL_IRQ 53
#elif defined(BOARD_imx8mq_evk) || defined(BOARD_maaxboard)
#define SERIAL_IRQ 58
#else
#error Need to define serial interrupt
#endif

/* Data for the interf baremetal boot image */
extern char _guest_bare_image[];
extern char _guest_bare_image_end[];

/* Microkit will set this variable to the start of the guest RAM memory region. */
uintptr_t guest_ram_vaddr;
// uintptr_t cyclecounters_vaddr;

// struct bench *bench = (void *)(uintptr_t)0x5010000;

// uint64_t start;
// uint64_t idle_ccount_start;
// uint64_t idle_overflow_start;

static void serial_ack(size_t vcpu_id, int irq, void *cookie)
{
    /*
     * For now we by default simply ack the serial IRQ, we have not
     * come across a case yet where more than this needs to be done.
     */
    microkit_irq_ack(SERIAL_IRQ_CH);
}

uintptr_t load_kernel(char *kernel_image, size_t kernel_size)
{
    uintptr_t kernel_dest = guest_ram_vaddr;

    LOG_VMM("Copying guest kernel image to 0x%x (0x%x bytes)\n", kernel_dest, kernel_size);

    memcpy((char *)kernel_dest, kernel_image, kernel_size);

    long checksum = 0;
    for (int i = 0; i < kernel_size; i++)
    {
        checksum += ((char *)kernel_dest)[i];
    }
    LOG_VMM("Checksum: %ld\n", checksum);
    return kernel_dest;
}

void init(void)
{
#ifdef CONFIG_PROFILER_ENABLE
    seL4_ProfilerRegisterThread(1);
#endif

    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", microkit_name);
    /* Place all the binaries in the right locations before starting the guest */
    size_t kernel_size = _guest_bare_image_end - _guest_bare_image;

    LOG_VMM("Invalidating cache for guest RAM at 0x%x (0x%x bytes)\n", guest_ram_vaddr, GUEST_RAM_SIZE);

    uintptr_t kernel_pc = (uintptr_t)load_kernel(_guest_bare_image, kernel_size);

    // seL4_ARM_VSpace_CleanInvalidate_Data(3, guest_ram_vaddr, guest_ram_vaddr + GUEST_RAM_SIZE);
    // seL4_ARM_VSpace_Unify_Instruction(3, guest_ram_vaddr, guest_ram_vaddr + GUEST_RAM_SIZE);

    if (!kernel_pc)
    {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }
    /* Initialise the virtual GIC driver */
    bool success = virq_controller_init(GUEST_VCPU_ID);
    if (!success)
    {
        LOG_VMM_ERR("Failed to initialise emulated interrupt controller\n");
        return;
    }
    // @ivanv: Note that remove this line causes the VMM to fault if we
    // actually get the interrupt. This should be avoided by making the VGIC driver more stable.

    // LOG_VMM("Registering IRQ %d on vCPU %d\n", SERIAL_IRQ, GUEST_VCPU_ID);
    // success = virq_register(GUEST_VCPU_ID, SERIAL_IRQ, &serial_ack, NULL);
    // if (!success) {
    //     LOG_VMM_ERR("Failed to register serial IRQ\n");
    //     return;
    // }
    // /* Just in case there is already an interrupt available to handle, we ack it here. */
    // microkit_irq_ack(SERIAL_IRQ_CH);

    // microkit_notify(START_PMU);

    // start = bench->ts;
    // idle_ccount_start = bench->ccount;
    // idle_overflow_start = bench->overflows;

    /* Finally start the guest */
    sel4bench_init();

    microkit_notify(10);

    guest_start(GUEST_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
}

void notified(microkit_channel ch)
{
    LOG_VMM("Got notified on channel %d\n", ch);
    switch (ch)
    {
    case SERIAL_IRQ_CH:
    {
        bool success = virq_inject(GUEST_VCPU_ID, SERIAL_IRQ);
        if (!success)
        {
            LOG_VMM_ERR("IRQ %d dropped on vCPU %d\n", SERIAL_IRQ, GUEST_VCPU_ID);
        }
        break;
    }
    default:
        printf("Unexpected channel, ch: 0x%lx\n", ch);
    }
}

/*
 * The primary purpose of the VMM after initialisation is to act as a fault-handler,
 * whenever our guest causes an exception, it gets delivered to this entry point for
 * the VMM to handle.
 */

void fault(microkit_id id, microkit_msginfo msginfo)
{
    ccnt_t before, after;
    before = get_cycles();
    bool success = fault_handle(id, msginfo);
    after = get_cycles();
    add_event(after - before, cur_event, LibVMM_Total_Event);

    if (success)
    {
        /* Now that we have handled the fault successfully, we reply to it so
         * that the guest can resume execution. */
        // before = sel4bench_get_cycle_count();
        // microkit_fault_reply(microkit_msginfo_new(0, 0));
        // after = sel4bench_get_cycle_count();
        // add_event(after - before, cur_event, Microkit_Reply);
        have_reply = true;
        reply_tag = microkit_msginfo_new(0, 0);
    }
    else
    {
        // printf("rut roh!\n");
        // uint64_t total = 0, idle = 0;
        // total = bench->ts - start;
        // total += (__LONG_MAX__ * 2UL + 1UL) * (bench->overflows - idle_overflow_start);
        // idle = bench->ccount - idle_ccount_start;

        // printf("Total: %lu\n", total);
        // printf("Idle: %lu\n", idle);

        // microkit_notify(STOP_PMU);
        microkit_notify(20);
    }
}
