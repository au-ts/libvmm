<!--
     Copyright 2024, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# A VMM managing a guest with multiple vCPUs

This example is the exact same as the [simple example](../simple) except
that each guest has multiple virtual CPUs instead of just one (defaults
to 4 vCPUs).

The example currently works on the following platforms:

* QEMU virt AArch64
* HardKernel Odroid-C4
* Avnet MaaXBoard

## Building

```sh
make MICROKIT_BOARD=<BOARD> MICROKIT_SDK=/path/to/sdk
```

Where `<MICROKIT_BOARD>` is one of:

* `qemu_virt_aarch64`
* `odroidc4`
* `maaxboard`

Other configuration options can be passed to the Makefile such as `MICROKIT_CONFIG`
and `BUILD_DIR`, see the Makefile for details.

If you would like to simulate the QEMU board you can run the following command:
```sh
make MICROKIT_BOARD=qemu_virt_aarch64 MICROKIT_SDK=/path/to/sdk qemu
```

This will build the example code as well as run the QEMU command to simulate a
system running the whole system.

By default the build system fetches the Linux kernel and initrd images from
Trustworthy Systems' website on-demand. To override this anduse your own images,
specify `LINUX` and/or `INITRD`. For example:

```sh
make MICROKIT_BOARD=qemu_virt_aarch64 MICROKIT_SDK=/path/to/sdk LINUX=/path/to/linux INITRD=/path/to/initrd qemu
```
