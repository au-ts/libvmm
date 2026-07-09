<!--
     Copyright 2026, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# A simple VMM for running Linux guests with UEFI

This example is a minimal VMM that supports booting the
TianoCore EDK II Open Virtual Machine Firmware (OVMF), that then boots a
Linux guest and a basic buildroot/BusyBox root file system. This gives a
basic command-line with some common Linux utilities.

The example currently works on the following platforms:

* x86_64

## Building

```sh
make MICROKIT_BOARD=<BOARD> MICROKIT_SDK=/path/to/sdk
```

Where `<MICROKIT_BOARD>` is one of:

* `x86_64_generic_vtx`

Other configuration options can be passed to the Makefile such as `MICROKIT_CONFIG`
and `BUILD_DIR`, see the Makefile for details.

If you would like to simulate the QEMU board you can run the following command:
```sh
make MICROKIT_BOARD=qemu_virt_aarch64 MICROKIT_SDK=/path/to/sdk qemu
```

This will build the example code as well as run the QEMU command to simulate a
system running the whole system.

By default the build system fetches the UEFI firmware, Linux kernel and initrd images from
Trustworthy Systems' website on-demand. To override this anduse your own images,
specify `FIRMWARE`, `LINUX` and/or `INITRD`. For example:

```sh
make MICROKIT_BOARD=qemu_virt_aarch64 MICROKIT_SDK=/path/to/sdk LINUX=/path/to/linux INITRD=/path/to/initrd FIRMWARE=/path/to/firmware qemu
```

## x86_64 Hardware Requirements

You will need an x86_64 Intel CPU with virtualisation (VT-x) turned
on in your BIOS.

The same applies for QEMU virtualisation, since QEMU's Tiny Code
Generator (TCG) does not emulate Intel's virtualisation extension
on x86_64. Whereas for ARM it does. Hence to run this example on
QEMU, you will need a Linux install and KVM enabled.
