# seL4CP VMM guide

## Supported platforms

On AArch64, the following platforms are supported:
* QEMU ARM virt
* Odroid-C2
* i.MX8MM-EVK

If your desired platform is not supported yet, please see the section on [adding your own platform support](#adding-platform-support).

## Creating a virtual machine

The first step before writing code is to have a system description that contains a virtual machine.

The following is essentially what is in [the QEMU example system](../board/qemu_arm_virt_hyp/systems/simple.system).
It is a fairly minimal system description to have a VMM start and execute a guest on AArch64.

I will now explain each part of this so that you too can understand why it is necessary.

First we create a VMM as a root PD. It is a root PD as we need the ability to be able to
start (and potentially stop and restart) the guest once we have initialised the system.

You will see that three memory regions (MRs) exist in the system.
1. Guest RAM region
2. UART serial
3. GIC VCPU

### Guest RAM region

Since the guest does not necessarily know it is being virtualised, it will expect some view
of contigious RAM that it can use. In this example system, we decide to give the guest 512MiB
of RAM, however you can provide however much is necessary for the guest. Essentially there needs
to be enough RAM to place the kernel image and any other associated binaries as well as enough
memory for the guest to actually do its job as an operating system.

This region is mapped into the VMM so that is can copy in the kernel image/any other binaries
and is of course mapped into the virtual machine so that it has access to its own RAM.

### UART serial

The UART device is passed through to the VM so that it can access it without trapping into the
seL4 kernel/VMM. This is done for performance and simplicity so that the VMM does not have to
emulate accesses to the UART device. Note that this works since nothing else is concurrently
accessing the UART. If say, for example, we had two VMs, both accessing the same UART. Mapping
both into the 

```xml
<system>
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
</system>
```

## Adding platform support

Before you can port the VMM to your desired platform you must have the following:

* A working port of the seL4 kernel.
* A working port of the seL4 Core Platform.

### AArch64 platform

Since the GIC (Generic Interrupt Controller) is virtualised, you must check that the driver in the VMM
and hardware virtualisation supports your platform.

The VMM supports both GIC version 2 and 3. This means that your platform must also either have a GICv2 or GICv3.

As stated above, the GIC VCPU device needs to be mapped into the guest, so you will first have to know the address
of that. The VMM code will also need this address, however, it will look at the DTS (Device Tree Source)
provided to the build system find it.

The rest is TODO, sorry :)
