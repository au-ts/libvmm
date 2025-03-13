<!--
     Copyright 2024, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# libvmm

The purpose of this project is to make it easy to run virtual machines on top of the seL4 microkernel.

This project contains three parts:
* `src/`: The source code of libvmm, a library for virtual-machine-monitors (VMM) to create and manage virtual machines on seL4.
* `examples/`: Examples for using libvmm.
* `tools/`: Tools that are useful when developing systems using virtual machines, but are not
  necessary for using the library.

This project is currently in-development and is frequently changing. It is not ready for
production use. The project also depends on the [seL4 Microkit](https://github.com/seL4/microkit)
SDK and expects to be used in a Microkit environment, in the future this may change such that libvmm
itself is environment agnostic.

For information on the project and how to use it, please see the [manual](docs/MANUAL.md).

## Getting started

To quickly show off the project, we will run the `simple` example. This example is
intended to simply boot a Linux guest that has serial input and output.

### Dependencies

* GNU Make
* Device Tree Compiler
* Clang/LLVM tools
* QEMU
* Microkit SDK (version 1.4.1)

For the Microkit SDK, you can download it from [here](https://github.com/seL4/microkit/releases/tag/1.4.1).

For all other dependencies, see the below instructions depending on your machine.

#### Ubuntu/Debian (apt):

```sh
sudo apt install -y make clang lld llvm qemu-system-arm device-tree-compiler
```

#### macOS (Homebrew):

If you do not have Homebrew installed, you can install it [here](https://brew.sh/).

It should be noted that while the examples in libvmm can be built
on macOS, if you need to do anything such as compile a custom Linux kernel image
or a guest root file system for developing your own system, you will probably have
less friction on a Linux machine.

```sh
# Note that you should make sure that the LLVM tools are in your path after running
# the install command. Homebrew does not do it automatically but does print out a
# message on how to do it.
brew install make qemu dtc llvm
```

#### Nix

There is a Nix flake available in the repository, so you can get a development shell via:
```sh
nix develop
```

Note that this will set the `MICROKIT_SDK` environment variable to the SDK path, you do not
need to download the Microkit SDK manually.

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
[    0.410421] Run /init as init process
[    0.410522]   with arguments:
[    0.410580]     /init
[    0.410627]   with environment:
[    0.410682]     HOME=/
[    0.410743]     TERM=linux
[    0.410788]     earlyprintk=serial
Starting syslogd: OK
Starting klogd: OK
Running sysctl: OK
Saving random seed: [    3.051374] random: crng init done
OK
Starting network: OK

Welcome to Buildroot
buildroot login:
```

The username to login is `root`. There is no password required.

## Next steps

Other examples are under `examples/`. Each example has its own documentation for
how to build and use it.

For more information, have a look at the [manual](docs/MANUAL.md).
