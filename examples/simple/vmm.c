/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <microkit.h>
#include <libvmm/guest.h>
#include <libvmm/virq.h>
#include <libvmm/util/util.h>
#include <libvmm/arch/aarch64/linux.h>
#include <libvmm/arch/aarch64/fault.h>

// benchmarking infra
#include <sddf/benchmark/sel4bench.h>
volatile int sel4bench_record_cycle_count(uint64_t type);
void benchmark_dump_array(void);
uint64_t counterArray[10000][2]; /* counter + tag */
int cnt = 0;
int _dump_done = 0;
int SAMPLE = 2000;

void benchmark_dump_array(void)
{
    for (int i=0;i<SAMPLE;i++) {
        printf("record %d: type: %llu: %llu\n", i, counterArray[i][1], counterArray[i][0]);
        if ((i+1)%2==0) {printf("---------------\n");}
    }
    printf("MKDONE\n");
}


volatile int sel4bench_record_cycle_count(uint64_t tag)
{
    if (cnt<SAMPLE) {
      counterArray[cnt][0] = sel4bench_get_cycle_count();
      counterArray[cnt][1] = tag;
      cnt++;
      return 0;
    }
    else if (!_dump_done) {
      benchmark_dump_array();
      _dump_done=1;
      return 0;
    }
    else {
      return 0;
    }
}


/*
 * As this is just an example, for simplicity we just make the size of the
 * guest's "RAM" the same for all platforms. For just booting Linux with a
 * simple user-space, 0x10000000 bytes (256MB) is plenty.
 */
#define GUEST_RAM_SIZE 0x10000000

#if defined(BOARD_qemu_virt_aarch64)
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
#elif defined(BOARD_maaxboard)
#define GUEST_DTB_VADDR 0x4f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x4c000000
#else
#error Need to define guest kernel image address and DTB address
#endif

/* For simplicity we just enforce the serial IRQ channel number to be the same
 * across platforms. */
#define SERIAL_IRQ_CH 1

#if defined(BOARD_qemu_virt_aarch64)
#define SERIAL_IRQ 33
#elif defined(BOARD_odroidc2_hyp) || defined(BOARD_odroidc4)
#define SERIAL_IRQ 225
#elif defined(BOARD_rpi4b_hyp)
#define SERIAL_IRQ 57
#elif defined(BOARD_imx8mm_evk)
#define SERIAL_IRQ 59
#elif defined(BOARD_imx8mq_evk) || defined(BOARD_maaxboard)
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

static void serial_ack(size_t vcpu_id, int irq, void *cookie) {
    /*
     * For now we by default simply ack the serial IRQ, we have not
     * come across a case yet where more than this needs to be done.
     */
    microkit_irq_ack(SERIAL_IRQ_CH);
}

// devmem 0x1ff100000 >/dev/null
bool benchmark_handler(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data) {
    sel4bench_record_cycle_count(2);
    printf("benchmarking: cycle count: %llu\n", sel4bench_get_cycle_count());
    return true;
}

// devmem 0x1ff200000 >/dev/null
// shouldn't be needed
bool benchmark_activate(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data) {
    seL4_BenchmarkNullSyscall();
    LOG_VMM("benchmarking: received fault, setup PMU\n");
    return true;
}

// devmem 0x1ff300000 >/dev/null
bool benchmark_clean(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data)
{
    _dump_done=0;
    cnt = 0;
    return true;
}

// devmem 0x1ff400000 >/dev/null
bool benchmark_quick(size_t vcpu_id, size_t offset, size_t fsr, seL4_UserContext *regs, void *data)
{
    return true;
}

void init(void) {
    /* Initialise the VMM, the VCPU(s), and start the guest */
    LOG_VMM("starting \"%s\"\n", microkit_name);
    /* Place all the binaries in the right locations before starting the guest */
    size_t kernel_size = _guest_kernel_image_end - _guest_kernel_image;
    size_t dtb_size = _guest_dtb_image_end - _guest_dtb_image;
    size_t initrd_size = _guest_initrd_image_end - _guest_initrd_image;
    uintptr_t kernel_pc = linux_setup_images(guest_ram_vaddr,
                                      (uintptr_t) _guest_kernel_image,
                                      kernel_size,
                                      (uintptr_t) _guest_dtb_image,
                                      GUEST_DTB_VADDR,
                                      dtb_size,
                                      (uintptr_t) _guest_initrd_image,
                                      GUEST_INIT_RAM_DISK_VADDR,
                                      initrd_size
                                      );
    if (!kernel_pc) {
        LOG_VMM_ERR("Failed to initialise guest images\n");
        return;
    }
    /* Initialise the virtual GIC driver */
    bool success = virq_controller_init(GUEST_VCPU_ID);
    if (!success) {
        LOG_VMM_ERR("Failed to initialise emulated interrupt controller\n");
        return;
    }
    success = virq_register(GUEST_VCPU_ID, SERIAL_IRQ, &serial_ack, NULL);
    /* Just in case there is already an interrupt available to handle, we ack it here. */
    microkit_irq_ack(SERIAL_IRQ_CH);
    sel4bench_init();
    /* Setup handler */
    fault_register_vm_exception_handler(0x1ff100000, 0x1000, benchmark_handler, NULL);
    fault_register_vm_exception_handler(0x1ff200000, 0x1000, benchmark_activate, NULL);
    fault_register_vm_exception_handler(0x1ff300000, 0x1000, benchmark_clean, NULL);
    fault_register_vm_exception_handler(0x1ff400000, 0x1000, benchmark_quick, NULL);
    printf("============ init benchmakring... ==================\n");
    printf("benchmarking: cycle count: %d\n", sel4bench_get_cycle_count());
    /* Finally start the guest */
    guest_start(GUEST_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
}

void notified(microkit_channel ch) {
    switch (ch) {
        case SERIAL_IRQ_CH: {
            bool success = virq_inject(GUEST_VCPU_ID, SERIAL_IRQ);
            if (!success) {
                LOG_VMM_ERR("IRQ %d dropped on vCPU %d\n", SERIAL_IRQ, GUEST_VCPU_ID);
            }
            break;
        }
        default:
            printf("Unexpected channel, ch: 0x%lx\n", ch);
    }
}

/*
 * The primary purpose of the VMM after initialisation is to act as a fault-handler.
 * Whenever our guest causes an exception, it gets delivered to this entry point for
 * the VMM to handle.
 */
int _tag_linuxfixup = 0;
int _tag_VMFault = 0;
seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo) {

    if (microkit_msginfo_get_label(msginfo) == seL4_Fault_VMFault) {
        _tag_VMFault = 1;
        sel4bench_record_cycle_count(1);
    }

    if (_tag_linuxfixup == 0) {
        seL4_BenchmarkNullSyscall();
        _tag_linuxfixup = 1;
    }

    bool success = fault_handle(child, msginfo);
    if (success) {
        /* Now that we have handled the fault successfully, we reply to it so
         * that the guest can resume execution. */
        *reply_msginfo = microkit_msginfo_new(0, 0);
        return seL4_True;
    }

    return seL4_False;
}
