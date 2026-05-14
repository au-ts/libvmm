<!--
     Copyright 2024, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Virtual machine Library
The virtual machine library provides the following interfaces:

```c
uint64_t linux_setup_images(uint64_t ram_start_gpa,
                            uintptr_t kernel,
                            size_t kernel_size,
                            uintptr_t dtb_src,
                            uint64_t dtb_dest_gpa,
                            size_t dtb_size,
                            uintptr_t initrd_src,
                            uint64_t initrd_dest_gpa,
                            size_t initrd_size)
```

`linux_setup_images` is used to copy Linux kernel, initial RAM disk
and device tree into guest RAM ready for execution. The first
argument *ram\_start\_gpa* is the guest physical address of the guest RAM.

The memory pointed to by *kernel* is checked to see that it is a Linux
kernel image before it is copied into an appropriate place in the guest RAM.

*dtb\_dest\_gpa* and *initrd\_dest\_gpa* should match what is in the Device
Tree.

`linux_setup_images()` returns the kernel entry point as guest physical address
if successful, otherwise 0.


```c
bool virq_controller_init()
```
Call to initialise the virtual interrupt controller for a guest. This must be
done before calling `virq_register()`.

```c
bool virq_register(size_t vcpu_id, size_t virq_num, virq_ack_fn_t ack_fn, void *ack_data)
```
Register an interrupt with the virtual interrupt controller.
The arguments are:

  `vpcu_id` the virtual CPU number to devlvier interrupts to.

  `virq_num` the IRQ number to deliver.

  `ack_fn` a function to be called when the guest acknowledges the
  interrupt.

  `ack_data` a cookie to be passed to the `ack_fn` when called.


```c
bool virq_inject(int irq)
```

Inject an IRQ into the boot virtual CPU. Note that this API requires that
the IRQ has been registered (with virq_register).

```c
bool virq_inject_vcpu(size_t vcpu_id, int irq)
```

The same functionality as `virq_inject`, but will inject the virtual IRQ into a specific virtual CPU.

```c
bool virq_register_passthrough(size_t vcpu_id, size_t irq, microkit_channel irq_ch)
```

Tell the system that interrupt request `irq` is a hardware interrupt
that will be passed through to the guest.  `irq_ch` is the channel in
the System Description File that maps to that interrupt.

```c
bool virq_handle_passthrough(microkit_channel irq_ch)
```

Perform an interrupt injection for the interrupt registered with
`virq_register_passthrough()` for channel `irq_ch`.

```c
bool guest_start(uintptr_t kernel_pc, uintptr_t dtb, uintptr_t initrd)
```
Start a guest Linux system, by passing control to the kernel entry
point.

```c
void guest_stop()
```
Stop executing the guest.
