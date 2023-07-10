/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "../../util/util.h"
#include "linux.h"

/*
 * The structure of the kernel image header and the magic value comes from the
 * Linux kernel documentation that can be found in the Linux source code. The
 * path is Documentation/arm64/booting.rst or Documentation/arch/arm64/booting.rst
 * depending on the version of the source code.
 */

#define LINUX_IMAGE_MAGIC 0x644d5241

struct linux_image_header {
    uint32_t code0;       // Executable code
    uint32_t code1;       // Executable code
    uint64_t text_offset; // Image load offset, little endian
    uint64_t image_size;  // Effective Image size, little endian
    uint64_t flags;       // kernel flags, little endian
    uint64_t res2;        // reserved
    uint64_t res3;        // reserved
    uint64_t res4;        // reserved
    uint32_t magic;       // Magic number, little endian, "ARM\x64"
    uint32_t res5;        // reserved (used for PE COFF offset)
};

// bool linux_start(size_t boot_vcpu_id, uintptr_t kernel_pc, uintptr_t dtb, uintptr_t initrd) {
//     seL4_UserContext regs = {0};
//     regs.x0 = GUEST_DTB_VADDR;
//     regs.spsr = 5; // PMODE_EL1h
//     regs.pc = kernel_pc;
//     // Set all the TCB registers
//     seL4_Word err = seL4_TCB_WriteRegisters(
//         BASE_VM_TCB_CAP + boot_vcpu_id,
//         false, // We'll explcitly start the guest below rather than in this call
//         0, // No flags
//         SEL4_USER_CONTEXT_SIZE, // Writing to x0, pc, and spsr // @ivanv: for some reason having the number of registers here does not work... (in this case 2)
//         &regs
//     );
//     assert(err == seL4_NoError);
//     if (err != seL4_NoError) {
//         LOG_VMM_ERR("Failed to write registers to boot vCPU's TCB (id is 0x%lx), error is: 0x%lx\n", boot_vcpu_id, err);
//         return false;
//     }
//     // Set the PC to the kernel image's entry point and start the thread.
//     LOG_VMM("starting guest at 0x%lx, DTB at 0x%lx, initial RAM disk at 0x%lx\n",
//         regs.pc, regs.x0, initrd);
//     sel4cp_vm_restart(boot_vcpu_id, regs.pc);

//     return true;
// }

uintptr_t linux_setup_images(uintptr_t ram_start,
                             uintptr_t kernel,
                             size_t kernel_size,
                             uintptr_t dtb_src,
                             uintptr_t dtb_dest,
                             size_t dtb_size,
                             uintptr_t initrd_src,
                             uintptr_t initrd_dest,
                             size_t initrd_size)
{
    // First we inspect the kernel image header to confirm it is a valid image
    // and to determine where in memory to place the image.
    struct linux_image_header *image_header = (struct linux_image_header *) kernel;
    assert(image_header->magic == LINUX_IMAGE_MAGIC);
    if (image_header->magic != LINUX_IMAGE_MAGIC) {
        LOG_VMM_ERR("Linux kernel image magic check failed\n");
        return 0;
    }
    // Copy the guest kernel image into the right location
    uintptr_t kernel_dest = ram_start + image_header->text_offset;
    // This check is because the Linux kernel image requires to be placed at text_offset of
    // a 2MB aligned base address anywhere in usable system RAM and called there.
    // In this case, we place the image at the text_offset of the start of the guest's RAM,
    // so we need to make sure that the start of guest RAM is 2MiB aligned.
    assert((ram_start & ((1 << 20) - 1)) == 0);
    LOG_VMM("Copying guest kernel image to 0x%x (0x%x bytes)\n", kernel_dest, kernel_size);
    memcpy((char *)kernel_dest, (char *)kernel, kernel_size);
    // Copy the guest device tree blob into the right location
    // Linux does not allow the DTB to be greater than 2 megabytes in size.
    assert(dtb_size <= (1 << 21));
    if (dtb_size > (1 << 21)) {
        LOG_VMM_ERR("Linux expects size of DTB to be less than 2MB, DTB size is 0x%lx bytes\n", dtb_size);
        return 0;
    }
    // Linux expects the address of the DTB to be on an 8-byte boundary.
    assert(dtb_dest % 0x8 == 0);
    if (dtb_dest % 0x8) {
        LOG_VMM_ERR("Linux expects DTB address to be on an 8-byte boundary, DTB address is 0x%lx\n", dtb_dest);
        return 0;
    }
    LOG_VMM("Copying guest DTB to 0x%x (0x%x bytes)\n", dtb_dest, dtb_size);
    memcpy((char *)dtb_dest, (char *)dtb_src, dtb_size);
    // Copy the initial RAM disk into the right location
    // @ivanv: add checks for initrd according to Linux docs
    LOG_VMM("Copying guest initial RAM disk to 0x%x (0x%x bytes)\n", initrd_dest, initrd_size);
    memcpy((char *)initrd_dest, (char *)initrd_src, initrd_size);

    return kernel_dest;
}
