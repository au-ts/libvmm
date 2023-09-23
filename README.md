# libvmm

The purpose of this project is to make it easy to run virtual machines on top of the seL4 microkernel.

This project contains three parts:
* `src/`: The VMM library, for creating and managing virtual machines on seL4.
* `examples/`: Examples for using the VMM library
* `tools/`: Tools that are useful when developing systems using virtual machines, but are not
  necessary for using the library.

This project is currently in-development and is frequently changing. It is not ready for
production use. The project also depends on the [seL4 Microkit](https://github.com/seL4/microkit)
SDK and expects to be used in a Microkit environment, in the future this may change such that the VMM
library itself is environment agnostic.

For information on the project and how to use it, please see the [manual](docs/MANUAL.md).

## Getting started

To quickly show off the project, we will run the `simple` example. This example is
intended to simply boot a Linux guest that has serial input and output.

### Dependencies

* Make
* dtc (Device Tree Compiler)
* Clang, LLD, and `llvm-ar`
* QEMU (for simulating the example)
* Microkit SDK

It should be noted that while the examples in the VMM can be reproduced
on macOS, if you need to do anything such as compile a custom Linux kernel image
or a guest root file system for developing your own system, you will probably have
less friction on a Linux machine.

Using `apt`:

```sh
sudo apt update && sudo apt install -y make clang lld llvm qemu-system-arm device-tree-compiler
```

Using Homebrew on macOS:

If you do not have Homebrew installed, you can do so [here](https://brew.sh/).

```sh
# Note that make sure that the LLVM tools are in your path after install, Homebrew
# does not do it automatically but does print out a message on how to do it.
brew install make qemu dtc llvm
```

Using [Nix](https://nixos.org/):
```sh
# In the root of the repository
nix-shell --pure
```

Finally, you will need an experimental Microkit SDK.

* Currently virtualisation support and other patches that the VMM requires are
  not part of mainline Microkit. See below for instructions on
  acquiring the SDK.
* Upstreaming the required changes is in-progress.

#### Acquiring the SDK

For acquiring the SDK, you have two options.

1. Download a pre-built SDK (recommended)
2. Build the SDK yourself.

##### Option 1 - Download pre-built SDK

Go to this page: https://github.com/Ivan-Velickovic/microkit/actions/runs/6257526318 and scroll to the bottom.

Click on the artefact that matches the system you will be using the SDK on (e.g Linux x86-64 or macOS AArch64).
You will then have to unzip and untar it (unfortunately GitHub zips the archive no matter what).

##### Option 2 - Building the SDK

You will need a development version of the Microkit SDK source code. You can acquire it with the following command:
```sh
git clone https://github.com/Ivan-Velickovic/microkit.git --branch dev
```

From here, you can follow the instructions
[here](https://github.com/Ivan-Velickovic/microkit/tree/dev) to build the SDK.

If you have built the SDK then the path to the SDK should look something like
this: `microkit/release/microkit-sdk-1.2.6`.

### Building and running

Finally, we can simulate a basic system with a single Linux guest with the
following command. We want to run the `simple` example system in a `debug`
configuration for the QEMU ARM virt system.
```sh
make BOARD=qemu_arm_virt MICROKIT_SDK=/path/to/sdk qemu
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
