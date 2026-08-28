<!--
     Copyright 2024, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# libvmm

The purpose of this project is to make it easy to run Virtual Machines (VMs) on top of the seL4 microkernel.

This project contains three parts:

* `src/`: The source code of libvmm, a library for Virtual Machine Monitors (VMMs) to create and manage VMs on seL4.
* `examples/`: Examples for using libvmm.
* `tools/`: Tools that are useful when developing systems using VMs, but are not
  necessary for using the library.

This project is currently in-development and is frequently changing. It is not ready for
production use. The project also depends on the [seL4 Microkit](https://github.com/seL4/microkit)
SDK and expects to be used in a Microkit environment, in the future this may change such that libvmm
itself is environment agnostic.

For information on the project and how to use it, please see the [manual](docs/MANUAL.md).

## Architecture support

This library supports creating Linux VMs on aarch64 and x86-64.

When targeting x86-64, you can build the library and examples on any macOS/Linux machine,
but you will only be able to run or virtualise it on a host with an Intel x86-64 CPU and
VT-x enabled in your BIOS.

There is special hardware requirements when targeting ARM, for more details, please see
the [manual](docs/MANUAL.md) or the examples for more details.

## Dependencies

These are the required software packages to build libvmm:

* GNU Make
* Device Tree Compiler
* iASL: ACPI Source Language Optimizing Compiler/Disassembler
* Clang/LLVM tools
* QEMU
* Microkit SDK (version 2.3.0)

For the Microkit SDK, you can download it [here](https://github.com/seL4/microkit/releases/2.3.0).

For all other dependencies, see the below instructions depending on your machine.

### Ubuntu/Debian (apt):

```sh
sudo apt install -y make clang lld llvm qemu-system-arm qemu-system-x86 device-tree-compiler acpica-tools
```

### Arch (pacman):

```sh
sudo pacman -S make clang lld llvm qemu-system-aarch64 qemu-system-x86 dtc acpica
```

### macOS (Homebrew):

If you do not have Homebrew installed, you can install it [here](https://brew.sh/).

It should be noted that while the examples in libvmm can be built
on macOS, if you need to do anything such as compile a custom Linux kernel image
or a guest root file system for developing your own system, you will probably have
less friction on a Linux machine.

```sh
# Note that you should make sure that the LLVM tools are in your path after running
# the install command. Homebrew does not do it automatically but does print out a
# message on how to do it.
brew install make qemu dtc llvm acpica
```

### Nix

There is a Nix flake available in the repository, so you can get a development shell via:
```sh
nix develop
```

Note that this will set the `MICROKIT_SDK` environment variable to the SDK path, you do not
need to download the Microkit SDK manually.


## Getting started

To quickly show off the project, we will run the `simple` example. This example is
intended to simply boot a Linux guest that has serial input and output.

### Building and running

First, initialize the repository:

```sh
git clone https://github.com/au-ts/libvmm
cd libvmm
git submodule update --init
```

Finally, we can simulate a basic system with a single Linux guest with the
following commands:
```sh
cd examples/simple
make MICROKIT_BOARD=qemu_virt_aarch64 MICROKIT_SDK=/path/to/sdk qemu
```

You should see Linux booting and be greeted with the buildroot prompt:
```
...
[    0.527068] Run /init as init process
[    0.527250]   with arguments:
[    0.527485]     /init
[    0.527612]   with environment:
[    0.527802]     HOME=/
[    0.527924]     TERM=linux
[    0.542824] ln (57) used greatest stack depth: 13272 bytes left
Saving 256 bits of creditable seed for next boot
Starting syslogd: OK
Starting acpid: OK
Running sysctl: OK
Starting network: OK
Starting crond: OK
ssh-keygen: generating new host keys: RSA ECDSA ED25519
Starting sshd: OK

Welcome to Buildroot
buildroot login:
```

The username to login is `root`. There is no password required.

## Next steps

Other examples are under `examples/`. Each example has its own documentation for
how to build and use it.

For more information, have a look at the [manual](docs/MANUAL.md).
