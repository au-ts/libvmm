/* @ivanv: need to consider the case where not all of these are used! */
/* @probits is used to say that this section contains data. */
/* The attributes "aw" is to say that the section is allocatable and that it is writeable. */

#if defined(VM_KERNEL_IMAGE_PATH)
.section .guest_kernel_image, "aw", @progbits
.global _guest_kernel_image, _guest_kernel_image_end
_guest_kernel_image:
.incbin VM_KERNEL_IMAGE_PATH
_guest_kernel_image_end:
#endif

#if defined(VM_DTB_IMAGE_PATH)
.section .guest_dtb_image, "aw", @progbits
.global _guest_dtb_image, _guest_dtb_image_end
_guest_dtb_image:
.incbin VM_DTB_IMAGE_PATH
_guest_dtb_image_end:
#endif

#if defined(VM_INITRD_IMAGE_PATH)
.section .guest_initrd_image, "aw", @progbits
.global _guest_initrd_image, _guest_initrd_image_end
_guest_initrd_image:
.incbin VM_INITRD_IMAGE_PATH
_guest_initrd_image_end:
#endif
