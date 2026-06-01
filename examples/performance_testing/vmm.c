/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
//#include "sel4/syscall.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <libvmm/guest.h>
#include <libvmm/virq.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/aarch64/linux.h>
#include <libvmm/arch/aarch64/fault.h>

#include <sddf/benchmark/sel4bench.h>
#include <sel4/benchmark_utilisation_types.h>
#include <sel4/sel4.h>

/*
 * As this is just an example, for simplicity we just make the size of the
 * guest's "RAM" the same for all platforms. For just booting Linux with a
 * simple user-space, 0x10000000 bytes (256MB) is plenty.
 */
#define GUEST_RAM_SIZE 0x10000000

#if defined(BOARD_qemu_virt_aarch64)
#define GUEST_DTB_VADDR 0x4f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x4d000000
#elif defined(BOARD_odroidc4)
#define GUEST_DTB_VADDR 0x2f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x2d700000
#elif defined(BOARD_maaxboard)
#define GUEST_DTB_VADDR 0x4f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x4c000000
#else
#error Need to define guest kernel image address and DTB address
#endif

/* For simplicity we just enforce the serial IRQ channel number to be the same
 * across platforms. */
#define SERIAL_IRQ_CH 2

#if defined(BOARD_qemu_virt_aarch64)
#define SERIAL_IRQ 33
#elif defined(BOARD_odroidc4)
#define SERIAL_IRQ 225
#elif defined(BOARD_maaxboard)
#define SERIAL_IRQ 58
#else
#error Need to define serial interrupt
#endif

/* Data for the guest's kernel image. */
extern char _guest_kernel_image[];
extern char _guest_kernel_image_end[];
/* Data for the device tree to be passed to the kernel. */
extern char _guest_dtb_image[];
extern char _guest_dtb_image_end[];
/* Data for the initial RAM disk to be passed to the kernel. */
extern char _guest_initrd_image[];
extern char _guest_initrd_image_end[];
/* Microkit will set this variable to the start of the guest RAM memory region. */
uintptr_t guest_ram_vaddr;

static void serial_ack(size_t vcpu_id, int irq, void *cookie)
{
    /*
     * For now we by default simply ack the serial IRQ, we have not
     * come across a case yet where more than this needs to be done.
     */
    microkit_irq_ack(SERIAL_IRQ_CH);
}

ccnt_t counter_value;
counter_bitfield_t benchmark_bf;

void init(void)
{
    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", microkit_name);
    sel4bench_init();
    
    assert(serial_config_check_magic(&config));

    serial_queue_init(&tx_queue_handle, config.tx.queue.vaddr, config.tx.data.size, config.tx.data.vaddr);

    serial_putchar_init(config.tx.id, &tx_queue_handle);
    sddf_printf("Hello world! I am %s.\nPlease give me character!\n", sddf_get_pd_name());
    
    /* Place all the binaries in the right locations before starting the guest */
    size_t kernel_size = _guest_kernel_image_end - _guest_kernel_image;
    size_t dtb_size = _guest_dtb_image_end - _guest_dtb_image;
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;
    uintptr_t kernel_pc = linux_setup_images(guest_ram_vaddr, (uintptr_t)_guest_kernel_image, kernel_size,
                                             (uintptr_t)_guest_dtb_image, GUEST_DTB_VADDR, dtb_size,
                                             (uintptr_t)_guest_initrd_image, GUEST_INIT_RAM_DISK_VADDR, initrd_size);
    if (!kernel_pc) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }
    /* Initialise the virtual GIC driver */
    bool success = virq_controller_init();
    if (!success) {
        LOG_VMM_ERR("Failed to initialise emulated interrupt controller\n");
        return;
    }
    success = virq_register(GUEST_BOOT_VCPU_ID, SERIAL_IRQ, &serial_ack, NULL);
    /* Just in case there is already an interrupt available to handle, we ack it here. */
    microkit_irq_ack(SERIAL_IRQ_CH);
    /* Finally start the guest */
    guest_start(kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
}

#define CYCLES_LOG_SIZE 1000

static bool syscall = true;

void notified(microkit_channel ch)
{   
    static volatile uint64_t cycle_log[CYCLES_LOG_SIZE];
    static volatile uint64_t request_count = 0; 


    if (syscall) {
        syscall = false;
        #ifdef CONFIG_BENCHMARK_TRACK_UTILISATION
            seL4_BenchmarkNullSyscall();
        #endif
    }

    volatile uint64_t start_cycles;
    SEL4BENCH_READ_CCNT(start_cycles);
    
    
    switch (ch) {
    case SERIAL_IRQ_CH: {
        bool success = virq_inject(SERIAL_IRQ);
        if (!success) {
            LOG_VMM_ERR("IRQ %d dropped\n", SERIAL_IRQ);
        }
        break;
    }
    default:
        printf("Unexpected channel, ch: 0x%lx\n", ch);
    }

    volatile uint64_t end_cycles;
    SEL4BENCH_READ_CCNT(end_cycles);
    
    cycle_log[request_count] = end_cycles - start_cycles;
    request_count++;
    
    if (request_count >= CYCLES_LOG_SIZE) {
        // asm volatile("nop");
        request_count = 0;
        printf("\nNotified cycle count\n");
        for (int i = 0; i < CYCLES_LOG_SIZE; i++) {
            printf("%d: %lu\n", i, cycle_log[i]);
        }
    }
}

/*
 * The primary purpose of the VMM after initialisation is to act as a fault-handler.
 * Whenever our guest causes an exception, it gets delivered to this entry point for
 * the VMM to handle.
 */
seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo)
{
    // if (syscall) {
    //     syscall = false;
    //     #ifdef CONFIG_BENCHMARK_TRACK_UTILISATION
    //         seL4_BenchmarkNullSyscall();
    //     #endif
    // }

    bool success = fault_handle(child, msginfo);
    if (success) {
        /* Now that we have handled the fault successfully, we reply to it so
         * that the guest can resume execution. */
        *reply_msginfo = microkit_msginfo_new(0, 0);
        return seL4_True;
    }

    return seL4_False;
}
