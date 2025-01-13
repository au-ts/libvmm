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

#include <libvmm/virtio/virtio.h>
#include <sddf/serial/queue.h>
#include <serial_config.h>
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
#define GUEST_DTB_VADDR 0x7f000000
#define GUEST_INIT_RAM_DISK_VADDR 0x7c000000
#else
#error Need to define guest kernel image address and DTB address
#endif

#define CLK_CNTL_PADDR_START 0x30380000
#define CLK_CNTL_PADDR_END 0x3038d000
#define CLK_CNTL_MR_SIZE 0xd000
#define CLK_ANALOG_PADDR_START 0x30360000
#define CLK_ANALOG_PADDR_END 0x30361000
#define CLK_ANALOG_MR_SIZE 0x1000

uintptr_t clk_analog_void;
uintptr_t clk_void;

/* For simplicity we just enforce the serial IRQ channel number to be the same
 * across platforms. */
#define SERIAL_IRQ_CH 1

/* Interrupts */
#define ETH_IRQ 152
#define ETH_IRQ_CH 10

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

/* Virtio Console */
#define SERIAL_VIRT_TX_CH 3
#define SERIAL_VIRT_RX_CH 4
#define VIRTIO_CONSOLE_IRQ (74)
#define VIRTIO_CONSOLE_BASE (0x130000)
#define VIRTIO_CONSOLE_SIZE (0x1000)

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

serial_queue_t *serial_rx_queue;
serial_queue_t *serial_tx_queue;

char *serial_rx_data;
char *serial_tx_data;

static struct virtio_console_device virtio_console;

bool clk_vmfault_handler(size_t vcpu_id, uintptr_t addr, size_t fsr, seL4_UserContext *regs, void *data) {
    uintptr_t phys_addr = addr + CLK_CNTL_PADDR_START;
    if (fault_is_read(fsr)) {
        uint32_t *target_phys_vaddr = (uint32_t *)phys_addr;
        asm volatile("" : : : "memory");
        uint64_t phys_data = *target_phys_vaddr;
        asm volatile("" : : : "memory");

        uint32_t *target_void_vaddr = (uint32_t *)(clk_void + addr);
        asm volatile("" : : : "memory");
        uint64_t void_data = *target_void_vaddr;
        asm volatile("" : : : "memory");

        fault_emulate_write(regs, phys_addr, fsr, phys_data);
    } else {
        uint64_t mask = fault_get_data_mask(addr, fsr);
        uint64_t data = fault_get_data(regs, fsr);

        uint32_t *target_void_vaddr = (uint32_t *)(clk_void + addr);
        asm volatile("" : : : "memory");
        *target_void_vaddr = (uint32_t)(data & mask);
        asm volatile("" : : : "memory");

        uint32_t *target_phys_vaddr = (uint32_t *)phys_addr;
        asm volatile("" : : : "memory");
        uint64_t phys_data = *target_phys_vaddr;
        asm volatile("" : : : "memory");

        if (phys_data != data) {
            /* printf("CLK|NATIVE: vaddr(0x%llx) data(0x%lx) mask(0x%llx)\n", phys_addr, phys_data, mask); */
            /* printf("CLK|WRITE: vaddr(0x%llx) data(0x%lx) mask(0x%llx)\n", phys_addr, data, mask); */
        } else {
            /* printf("CLK|MATCHED WRITE: vaddr(0x%llx) data(0x%lx) mask(0x%llx)\n", phys_addr, data, mask); */
        }
    }

    return true;
}

bool clk_analog_vmfault_handler(size_t vcpu_id, uintptr_t addr, size_t fsr, seL4_UserContext *regs, void *data) {
    uintptr_t phys_addr = addr + CLK_ANALOG_PADDR_START;
    if (fault_is_read(fsr)) {
        uint32_t *target_phys_vaddr = (uint32_t *)phys_addr;
        asm volatile("" : : : "memory");
        uint64_t phys_data = *target_phys_vaddr;
        asm volatile("" : : : "memory");

        /* uint32_t *target_void_vaddr = (uint32_t *)(clk_analog_void + addr); */
        /* asm volatile("" : : : "memory"); */
        /* uint64_t void_data = *target_void_vaddr; */
        /* asm volatile("" : : : "memory"); */

        fault_emulate_write(regs, phys_addr, fsr, phys_data);
    } else {
        uint64_t mask = fault_get_data_mask(addr, fsr);
        uint64_t data = fault_get_data(regs, fsr);

        uint32_t *target_void_vaddr = (uint32_t *)(clk_analog_void + addr);
        asm volatile("" : : : "memory");
        *target_void_vaddr = (uint32_t)(data & mask);
        asm volatile("" : : : "memory");

        uint32_t *target_phys_vaddr = (uint32_t *)phys_addr;
        asm volatile("" : : : "memory");
        uint64_t phys_data = *target_phys_vaddr;
        asm volatile("" : : : "memory");

        if (phys_data != data) {
            /* printf("CLK_ANALOG|NATIVE: vaddr(0x%llx) data(0x%lx) mask(0x%llx)\n", phys_addr, phys_data, mask); */
            /* printf("CLK_ANALOG|WRITE: vaddr(0x%llx) data(0x%lx) mask(0x%llx)\n", phys_addr, data, mask); */
        } else {
            /* printf("CLK_ANALOG|MATCHED WRITE: vaddr(0x%llx) data(0x%lx) mask(0x%llx)\n", phys_addr, data, mask); */
        }
    }

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
    if (!virq_register_passthrough(GUEST_VCPU_ID, ETH_IRQ, ETH_IRQ_CH)) {
        LOG_VMM_ERR("Failed to pass thru ETH irq\n");
        return;
    }

    /* Trap all access to clk into the hypervisor for emulation */
    bool clk_fault_reg_ok = fault_register_vm_exception_handler(CLK_CNTL_PADDR_START, CLK_CNTL_MR_SIZE, &clk_vmfault_handler, NULL);
    if (!clk_fault_reg_ok) {
        LOG_VMM_ERR("Failed to register the VM fault handler for clk\n");
        return;
    }
    clk_fault_reg_ok = fault_register_vm_exception_handler(CLK_ANALOG_PADDR_START, CLK_ANALOG_MR_SIZE, &clk_analog_vmfault_handler, NULL);
    if (!clk_fault_reg_ok) {
        LOG_VMM_ERR("Failed to register the VM fault handler for clk analog\n");
        return;
    }
    /* Just in case there is already an interrupt available to handle, we ack it here. */
    /* microkit_irq_ack(SERIAL_IRQ_CH); */
    serial_queue_handle_t serial_rxq, serial_txq;
    serial_cli_queue_init_sys(microkit_name, &serial_rxq, serial_rx_queue, serial_rx_data, &serial_txq, serial_tx_queue,
                              serial_tx_data);

    success = virtio_mmio_console_init(&virtio_console,
                                       VIRTIO_CONSOLE_BASE,
                                       VIRTIO_CONSOLE_SIZE,
                                       VIRTIO_CONSOLE_IRQ,
                                       &serial_rxq, &serial_txq,
                                       SERIAL_VIRT_TX_CH);
    if (!success) {
        LOG_VMM_ERR("Failed to initialise virtio console\n");
        return;
    }

    /* Finally start the guest */
    guest_start(GUEST_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
    LOG_VMM("VMM is ready.\n");
}

void notified(microkit_channel ch) {
    bool handled = false;

    handled = virq_handle_passthrough(ch);
    switch (ch) {
    case SERIAL_VIRT_RX_CH: {
        /* We have received an event from the serial virtualiser, so we
         * call the virtIO console handling */
        virtio_console_handle_rx(&virtio_console);
        handled = true;
        break;
    }
    default:
        break;
    }

    if (!handled) {
        LOG_VMM_ERR("Unhandled notification on channel %d\n", ch);
    }
}

/*
 * The primary purpose of the VMM after initialisation is to act as a fault-handler.
 * Whenever our guest causes an exception, it gets delivered to this entry point for
 * the VMM to handle.
 */
seL4_Bool fault(microkit_child child, microkit_msginfo msginfo, microkit_msginfo *reply_msginfo) {
    bool success = fault_handle(child, msginfo);
    if (success) {
        /* Now that we have handled the fault successfully, we reply to it so
         * that the guest can resume execution. */
        *reply_msginfo = microkit_msginfo_new(0, 0);
        return seL4_True;
    }

    return seL4_False;
}
