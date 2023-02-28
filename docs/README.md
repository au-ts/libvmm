# seL4CP VMM guide

## Supported platforms

On AArch64, the following platforms are supported:
* QEMU ARM virt
* Odroid-C2
* i.MX8MM-EVK

If your desired platform is not supported yet, please see the section on
[adding your own platform support](#adding-platform-support).

## Creating a virtual machine

The first step before writing code is to have a system description that contains
a virtual machine.

The following is essentially what is in
[the QEMU example system](../board/qemu_arm_virt_hyp/systems/simple.system),
it is a fairly minimal system description to have a VMM start and execute a
guest on AArch64:

```xml
<memory_region name="guest_ram" size="0x10_000_000" />
<memory_region name="serial" size="0x1_000" phys_addr="0x9000000" />
<memory_region name="gic_vcpu" size="0x1_000" phys_addr="0x8040000" />

<protection_domain name="VMM" priority="254">
    <program_image path="vmm.elf" />
    <map mr="guest_ram" vaddr="0x40000000" perms="rw" setvar_vaddr="guest_ram_vaddr" />
    <virtual_machine name="linux" vm_id="1">
        <map mr="guest_ram" vaddr="0x40000000" perms="rwx" />

        <map mr="serial" vaddr="0x9000000" perms="rw" />
        <map mr="gic_vcpu" vaddr="0x8010000" perms="rw" />
    </virtual_machine>
</protection_domain>
```

I will now explain each part of this so that you too can understand why it is
necessary.

First we create a VMM as a root PD. It is a root PD as we need the ability to be
able to start (and potentially stop and restart) the guest once we have
initialised the system.

You will see that three memory regions (MRs) exist in the system.
1. `guest_ram` for the guest's RAM region
2. `serial` for the UART
3. `gic_vcpu` for the Generic Interrupt Controller VCPU interface

### Guest RAM region

Since the guest does not necessarily know it is being virtualised, it will
expect some view of contigious RAM that it can use. In this example system, we
decide to give the guest 512MiB of RAM, however you can provide however much is
necessary for the guest. Essentially there needs to be enough RAM to place the
kernel image and any other associated binaries as well as enough memory for the
guest to actually do its job as an operating system.

This region is mapped into the VMM so that is can copy in the kernel image/any
other binaries and is of course mapped into the virtual machine so that it has
access to its own RAM.

### UART serial

The UART device is passed through to the VM so that it can access it without
trapping into the seL4 kernel/VMM. This is done for performance and simplicity
so that the VMM does not have to emulate accesses to the UART device. Note that
this works since nothing else is concurrently accessing the UART. If say, for
example, we had two VMs, both accessing the same UART. Mapping both into the

### GIC VCPU interface

## Adding platform support

Before you can port the VMM to your desired platform you must have the following:

* A working port of the seL4 kernel in hypervisor mode
* A working port of the seL4 Core Platform where the kernel is built as a
  hypervisor

### AArch64 platform

#### Guest setup

While in theory you should be able to run any guest, the VMM has only been tested
with Linux.

Required binaries:

* Linux kernel image
* Linux device tree
* Initial RAM disk

It is possible to package all of these up into one single binary, but the defualt
Linux kernel configuration does not do this. You can look at some of the examples
as to how they are setup. Currently, the VMM is expecting there to be three
separate images, it is a TODO to fix this.

Before attempting to get the VMM working, I strongly encourage you to make sure
that these binaries work natively, as in, without being virtualised. If they do
not, they probably will not work with the VMM either.

### Add init RAM disk addr to device tree

#### Generic Interrupt Controller (GIC)

GIC version 2 and 3 are not fully virtualised by the hardware and therefore the
interrupt controller is partially virtualised in the VMM. Check that your
platform supports either GICv2 or GICv3.

As stated above, the GIC VCPU interface needs to be mapped into the guest, so
you will first have to know the address of that. The VMM code will also need
this address, however, it will look at the DTS (Device Tree Source) provided to
the build system find it.

The rest is TODO, sorry :)
