# VMM on the seL4 Core Platform

This is a Virtual Machine Monitor (VMM) built on the seL4 Core Platform (seL4CP).
The purpose of the VMM is to make it easy to run virtual machines on top of the seL4 microkernel.

It is (at least initially) a port of the already existing
[CAmkES VMM](https://github.com/sel4/camkes-vm-examples). See the bottom of the
README for progress/planned features.

Due to being a work-in-progress, expect frequent changes to the VMM as well as the
SDK used by the VMM.

For information on how to use the VMM, supported platforms, how to add your own
platform, etc, please see the [manual](docs/MANUAL.md).

## Getting started

This project is able to be compiled on Linux x86-64, macOS x86-64 and
macOS AArch64.

However, it should be noted that while the examples in the VMM can be reproduced
on macOS, if you need to do anything such as compile a custom Linux kernel image
or a guest root file system, you will probably have less friction on a Linux machine.

### Dependencies

* Make
* dtc (Device Tree Compiler)
* AArch64 cross compiler toolchain
* QEMU (if you wish to simulate the VMM)

Using `apt`:

```sh
sudo apt update
sudo apt install make gcc-aarch64-linux-gnu qemu-system-arm device-tree-compiler
```

Using Homebrew:

```sh
# Homebrew does not provide ARM cross compilers by default, so we use
# this repository (https://github.com/messense/homebrew-macos-cross-toolchains).
brew tap messense/macos-cross-toolchains
brew install make qemu aarch64-unknown-linux-gnu dtc
```

Using [Nix](https://nixos.org/):
```sh
# In the root of the repository
nix-shell --pure
```

Finally, you will need an experimental seL4 Core Platform SDK.

* Currently virtualisation support and other patches that the VMM requires are
  not part of mainline seL4 Core Platform. See below for instructions on
  acquiring the SDK.
* Upstreaming the required changes is in-progress.

#### Acquiring the SDK

For acquiring the SDK, you have two options.

1. Download a pre-built SDK (recommended)
2. Build the SDK yourself.

##### Option 1 - Download pre-built SDK

For Linux x86-64:
```sh
wget https://sel4.ivanvelickovic.com/downloads/sel4cp-sdk-dev-bf06734-linux-x86-64.tar.gz
# Check that the SDK hash is correct
echo "f4cee94bf0296084c123419644c8780e25fdb7610a4357d78765544a959c2b7b sel4cp-sdk-dev-bf06734-linux-x86-64.tar.gz" | sha256sum -c
# Untar the SDK
tar xf sel4cp-sdk-dev-bf06734-linux-x86-64.tar.gz
```

For macOS x86-64:
```sh
wget https://sel4.ivanvelickovic.com/downloads/sel4cp-sdk-dev-bf06734-macos-x86-64.tar.gz
# Check that the SDK hash is correct
echo "f7cb671661bdabdac7055a21d508bf6029553fc3726f9ad0a30c6667a16c2825 sel4cp-sdk-dev-bf06734-macos-x86-64.tar.gz" | sha256sum -c
# Untar the SDK
tar xf sel4cp-sdk-dev-bf06734-macos-x86-64.tar.gz
```

For macOS AArch64:
```sh
wget https://sel4.ivanvelickovic.com/downloads/sel4cp-sdk-dev-bf06734-macos-aarch64.tar.gz
# Check that the SDK hash is correct
echo "987cdea178456c3db0c6bfd7a4b5c3dd878c39697f6ec2563d1eedc6c934eb85 sel4cp-sdk-dev-bf06734-macos-aarch64.tar.gz" | sha256sum -c
# Untar the SDK
tar xf sel4cp-sdk-dev-bf06734-macos-aarch64.tar.gz
```

##### Option 2 - Building the SDK

You will need a development version of the seL4CP SDK source code. You can acquire it with the following command:
```sh
git clone https://github.com/Ivan-Velickovic/sel4cp.git --branch dev
```

From here, you can follow the instructions
[here](https://github.com/Ivan-Velickovic/sel4cp/tree/dev) to build the SDK.

If you have built the SDK then the path to the SDK should look something like
this: `sel4cp/release/sel4cp-sdk-1.2.6`.

## Building the example system

Finally, we can simulate a basic system with a single Linux guest with the
following command. We want to run the `simple` example system in a `debug`
configuration for the QEMU ARM virt system.
```sh
make BOARD=qemu_arm_virt SEL4CP_SDK=/path/to/sel4cp-sdk-1.2.6 qemu
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

Other examples are under `board/$board/systems/`.

For more information on the VMM, have a look at the [manual](docs/MANUAL.md).

## Roadmap

The following is work planned to be done, there is currently no strict timeline
for any of this. If you would like something to be added that is not in this list
or have any other queries, feel free to open a GitHub issue!

* 64-bit RISC-V support
* 64-bit x86 support
* virtIO back-ends (console, socket, network, block)
* A performance evaluation
* Add CI to make sure all the example systems work
* GICv4 support to allow for direct injection of vLPIs.
* SMP support. This includes having one guest using multiple cores
  as well as pinning a guest to a specific core.
* Get RPi4B+ working
* Support QEMU ARM "virt" with GICv3
* Support QEMU ARM "virt" with other CPUs such as Cortex-A72
* Other, general improvements to the usability and extensibility of the VMM.
    * Particularly, configuring the VMs/VMM should not have significant friction.

Currently, there are no plans for 32-bit guests or 32-bit hosts.
