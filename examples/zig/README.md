<!--
     Copyright 2024, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Zig VMM example

This example aims to show using [Zig](https://ziglang.org/) to interact with
libvmm. Although Zig is most-often thought of as a programming language, the
project actually provides three things:
1. The Zig programming language
2. The Zig build system
3. A portable drop-in C/C++ cross-compiler

This example makes use of all three. The VMM code itself is written in the
Zig programming langauge, it calls into libvmm which has been compiled using
the Zig C compiler. Building the example is done via the Zig build system.

## Building the example

This example expects to be built with Zig 0.14.*.

You can download Zig [here](https://ziglang.org/download/).

```sh
zig build -Dsdk=/path/to/sdk
```

The build system fetches the Linux kernel and initrd images on-demand from
Trustworthy Systems' website. To override this and use your own images, you can
specify the paths with the `-Dlinux` and/or `-Dinitrd` options. For example:
```sh
zig build -Dsdk=/path/to/sdk -Dlinux=/path/to/linux -Dinitrd=/path/to/initrd
```

To view other options available when building the example (such as optimisation
level or the target Microkit configuration), run the following command:
```sh
zig build -Dsdk=/path/to/sdk -h
```

## Running the example

To also run the example via QEMU, run the following command:
```sh
zig build -Dsdk=/path/to/sdk qemu
```

You should see the following output after a couple of seconds:
```
Welcome to Buildroot
buildroot login:
```

The login is `root`. There is no password necessary. After logging in, you will
see the console prompt where you can input commands:
```
Welcome to Buildroot
buildroot login: root
#
```
