# seL4CP VMM manual

This document aims to describe the VMM and how to use it. If you feel there is
something missing from this document or the VMM itself, feel free to let us
know by [opening an issue on the GitHub repository](https://github.com/au-ts/sel4cp_vmm).

## Supported platforms

On AArch64, the following platforms are currently supported:
* QEMU ARM virt
* Odroid-C2
* Odroid-C4
* i.MX8MM-EVK

Support for other architectures is in-progress (RISC-V) and planned (x86).

Existing example systems built on these platforms can be found in the
repository under `board/$BOARD/systems/`.

If your desired platform is not supported yet, please see the section on
[adding your own platform support](#adding-platform-support).

## Creating a system

The goal of this section is to give a detailed introduction into making a
system using the VMM. This is done by explaining one of the example QEMU ARM
virt systems that boots up a simple Linux guest.

All the existing systems are located in `board/$BOARD/systems/`. This is
where the Makefile will look when you pass the `SYSTEM` argument.

### Specifying a virtual machine

<!-- @ivanv: Explain <virtual_machine> options etc more. -->

The first step before writing code is to have a system description that contains
a virtual machine and the VMM protection domain (PD).

The following is essentially what is in
[the QEMU example system](../board/qemu_arm_virt/systems/simple.system),

```xml
<memory_region name="guest_ram" size="0x10_000_000" />
<memory_region name="serial" size="0x1_000" phys_addr="0x9000000" />
<memory_region name="gic_vcpu" size="0x1_000" phys_addr="0x8040000" />

<protection_domain name="VMM" priority="254">
    <program_image path="vmm.elf" />
    <map mr="guest_ram" vaddr="0x40000000" perms="rw" setvar_vaddr="guest_ram_vaddr" />
    <virtual_machine name="linux" id="0">
        <map mr="guest_ram" vaddr="0x40000000" perms="rwx" />
        <map mr="serial" vaddr="0x9000000" perms="rw" />
        <map mr="gic_vcpu" vaddr="0x8010000" perms="rw" />
    </virtual_machine>
</protection_domain>
```

First we create a VMM as a root PD that contains a virtual machine (VM).
This hierarchy is necessary as the VMM needs to be able to access the guest's
TCB registers and VCPU registers for initialising the guest, delivering virtual
interrupts to the guest and restarting the guest.

You will also see that three memory regions (MRs) exist in the system.
1. `guest_ram` for the guest's RAM region
2. `serial` for the UART
3. `gic_vcpu` for the Generic Interrupt Controller VCPU interface

### Guest RAM region

Since the guest does not necessarily know it is being virtualised, it will
expect some view of contiguous RAM that it can use. In this example system, we
decide to give the guest 256MiB to use as "RAM", however you can provide
however much is necessary for your guest. At a bare minimum, there needs to be
enough memory to place the kernel image and any other associated binaries. How
much memory is required for it to function depends on what you intend to do
with the guest.

This region is mapped into the VMM so that it can copy in the kernel image and
any other binaries and is of course also mapped into the virtual machine so
that it has access to its own RAM.

We can see that the region is mapped into the VMM with
`setvar_vaddr="guest_ram_vaddr"`. The VMM expects that variable to contain
the starting address of the guest's RAM. With the current implementation of the
VMM, it expects that the virtual address of the guest RAM that is mapped into
the VMM as well as the guest physical address of the guest RAM to be the same.
This is done for simplicity at the moment, but could be changed in the future
if someone had a strong desire for the two values to not be coupled.

### Serial region

The UART device is passed through to the VM so that it can access it without
trapping into the seL4 kernel/VMM. This is done for performance and simplicity
so that the VMM does not have to emulate accesses to the UART device. Note that
this will work since nothing else is concurrently accessing the device.

### GIC virtual CPU interface region

The GIC VCPU interface region is a hardware device region passed through to the
guest. The device is at a certain physical address, which is then mapped into
the guest at the address of the GIC CPU interface that the guest expects. In the
case of the example above, the GIC VCPU interface is at `0x8040000`, and we map
this into the guest physical address of `0x8010000`, which is where the guest
expects the CPU interface to be. The rest of the GIC is virtualised in the VGIC
driver in the VMM. Like the UART, the address of the GIC is platform specific.

### Running the system

An example Make command looks something like this:
```sh
make BUILD_DIR=build \
     SEL4CP_SDK=/path/to/sdk \
     CONFIG=debug \
     BOARD=qemu_arm_virt \
     SYSTEM=simple.system \
     run
```

## Adding platform support

Before you can port the VMM to your desired platform you must have the following:

* A working platform port of the seL4 kernel in hypervisor mode.
* A working platform port of the seL4 Core Platform where the kernel is built as a
  hypervisor.

### Guest setup

While in theory you should be able to run any guest, the VMM has only been tested
with Linux and hence the following instructions are somewhat Linux specific.

Required guest files:

* Kernel image
* Device Tree Source to be passed to the kernel at boot
* Initial RAM disk

Each platform image directory (`board/$BOARD/images`) contains a README with
instructions on how to reproduce the images used if you would like to see
examples of how other example systems are setup.

Before attempting to get the VMM working, I strongly encourage you to make sure
that these binaries work natively, as in, without being virtualised. If they do
not, they likely will not work with the VMM either.

You can either add these images into `board/$BOARD/images/` or specify the
`IMAGE_DIR` argument when building the system which points to the directory
that contains all of the files.

#### Implementation notes

Currently, the VMM is expecting there to be three separate images although it
is possible to package all of these up into one single image, but the default
Linux kernel configuration does not do this. It would be possible to change the
VMM to allow this functionality.

The VMM also (for now) does not have the ability to generate a DTB, therefore
requiring the Device Tree Source at build time.

### Generic Interrupt Controller (GIC)

GIC version 2 and 3 are not fully virtualised by the hardware and therefore the
interrupt controller is partially virtualised in the VMM. Confirm that your
ARM platform supports either GICv2 or GICv3, as those are the versions that the
VMM supports.

### Add platform to VMM source code

<!-- @ivanv: These instructions could be improved -->

Lastly, there are some hard-coded values that the VMM needs to support a platform.
There are three files that need to be changed:

* `src/vmm.h`
* `src/vgic/vgic.h`
* For Linux, the device tree needs to contain the location of the initial RAM disk,
  see the `chosen` node of `board/qemu_arm_virt/images/linux.dts` as an example.

As you can probably tell, all this information that needs to be added is known at
build-time, the plan is to auto-generate these values that the VMM needs to make it
easier to add platform support (and in general make the VMM less fragile).
