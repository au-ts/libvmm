<!--
     Copyright 2024, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Virtual machine Library
The virtual machine library provides the following interfaces:

` uintptr_t linux_setup_images(uintptr_t` *ram\_start*,
                             uintptr_t *kernel*,
                             size_t *kernel\_size*,
                             uintptr_t *dtb\_src*,
                             uintptr_t *dtb\_dest*,
                             size_t *dtb\_size*,
                             uintptr_t *initrd\_src*,
                             uintptr_t *initrd\_dest*,
                             size_t *initrd\_size*)`
							 


`linux_setup_images` is used to copy Linux kernel, initial RAM disk
and device tree into guest RAM ready for execution.  The first
argument *ram\_start* is the virtual address of the guest RAM.

The memory pointed to by *kernel* is checked to see that it is a Linux
kernel `vmlinux`-style
image before it is copied into an appropriate place in the guest RAM.


*initrd\_dest* and *dtb\_dest* should match what is in the Device
Tree.

`linux_setup_images()` returns the kernel entry point if successful,
otherwise -1.


`bool virq_controller_init(size_t cpu_id)`
Call to initialise the interrupt controller for a guest. This must be
done before calling `virq_register()`

`bool virq_register(size_t vcpu_id, size_t virq_num, 
	virq_ack_fn_t ack_fn, 
	void *ack_data);`

Register an interrupt with the virtual interrupt controller.
The arguments are:

  `vpcu_id` the virtual CPU number to devlvier interrupts to

  `virq_num` the IRQ number to deliver
  
  `ack_fn` a function to be called when the guest acknowledges the
  interrupt
  
	`ack_data` a cookie to be passed to the `ack_fn` when called.


`bool virq_inject(size_t vcpu_id, int irq)`

Inject interrupt `irq` into the virtual interrupt controller on virtual
cpu `vcpu_id`

`bool virq_register_passthrough(size_t vcpu_id, size_t irq,
microkit_channel irq_ch);`

Tell the system that interrupt request `irq` is a hardware interrupt
that will be passed through to the guest.  `irq_ch` is the channel in
the System Description File that maps to that interrupt.

`bool virq_handle_passthrough(microkit_channel irq_ch)`
Perform an interrupt injection for the interrupt registered with 
`virq_register_passthrough()` for channel `irq_ch`

`bool guest_start(size_t boot_vcpu_id, uintptr_t kernel_pc, uintptr_t
dtb, uintptr_t initrd);`
Start a guest Linux system, by passing control to the kernel entry
point.

`void guest_stop(size_t boot_vcpu_id);`
Stop executing the guest.

`bool guest_restart(size_t boot_vcpu_id, uintptr_t guest_ram_vaddr,
size_t guest_ram_size);`
Restart the guest, possibly with a different image.
