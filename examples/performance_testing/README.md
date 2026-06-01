<!--
     Copyright 2024, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# A simple VMM for running Linux guests

This example is a minimal VMM that supports Linux guests and a basic
buildroot/BusyBox root file system. This gives a basic command-line with some
common Linux utilities.

The example currently works on the following platforms:

* QEMU virt AArch64
* HardKernel Odroid-C4
* Avnet MaaXBoard

## Building with Make

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

## Building with Zig

For educational purposes, you can also build and run this example using the
[Zig](https://ziglang.org/) build system.

This example expects to be built with Zig 0.15.x.

You can download Zig [here](https://ziglang.org/download/).

```sh
zig build -Dsdk=/path/to/sdk -Dboard=<MICROKIT_BOARD>
```

Where `<MICROKIT_BOARD>` is one of:

* `qemu_virt_aarch64`
* `odroidc4`
* `maaxboard`

If you are building for QEMU then you can also run QEMU by doing:
```sh
zig build -Dsdk=/path/to/sdk -Dboard=qemu_virt_aarch64 qemu
```

Similar with Make, the Zig build system will fetch Linux kernel and initrd images
Trustworthy Systems' website. To use your own images, specify `-Dlinux` and/or
`-Dinitrd`. For example:

```sh
zig build -Dsdk=/path/to/sdk -Dboard=qemu_virt_aarch64 -Dlinux=/path/to/linux -Dinitrd=/path/to/initrd qemu
```

You can view other options by doing:
```sh
zig build -Dsdk=/path/to/sdk -Dboard=<MICROKIT_BOARD> -h
```

