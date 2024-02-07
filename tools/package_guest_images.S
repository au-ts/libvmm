/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*
 * What even is this file? 
 *
 * The purpose of this file is to include binaries that the VMM needs to setup,
 * such as the Linux kernel image, the DTB, and the initial RAM disk. What
 * happens is that the assembler includes the binary data in an ELF section
 * that we specify. We can then have global variables that essentially point to
 * where the data has been included.
 *
 * For each ELF section, we define a name and also some configuration. The
 * attributes "aw" is to say that the section is allocatable and that it is
 * writeable. We also have "@progbits", this is to indicate that the section
 * contains data (rather than code or something else).
 *
 */  
 
#if defined(GUEST_KERNEL_IMAGE_PATH)
.section .guest_kernel_image, "aw", @progbits
.global _guest_kernel_image, _guest_kernel_image_end
_guest_kernel_image:
.incbin GUEST_KERNEL_IMAGE_PATH
_guest_kernel_image_end:
#endif

#if defined(GUEST_DTB_IMAGE_PATH)
.section .guest_dtb_image, "aw", @progbits
.global _guest_dtb_image, _guest_dtb_image_end
_guest_dtb_image:
.incbin GUEST_DTB_IMAGE_PATH
_guest_dtb_image_end:
#endif

#if defined(GUEST_INITRD_IMAGE_PATH)
.section .guest_initrd_image, "aw", @progbits
.global _guest_initrd_image, _guest_initrd_image_end
_guest_initrd_image:
.incbin GUEST_INITRD_IMAGE_PATH
_guest_initrd_image_end:
#endif

 
#if defined(GUEST_BARE_IMAGE_PATH)
.section .guest_bare_image, "aw", @progbits
.global _guest_bare_image, _guest_bare_image_end
_guest_bare_image:
.incbin GUEST_BARE_IMAGE_PATH
_guest_bare_image_end:
#endif

#if defined(GUEST_INTERF_IMAGE_PATH)
.section .guest_interf_image, "aw", @progbits
.global _guest_interf_image, _guest_interf_image_end
_guest_interf_image:
.incbin GUEST_INTERF_IMAGE_PATH
_guest_interf_image_end:
#endif


