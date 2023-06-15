# VMM on the seL4 Core Platform

This is a Virtual Machine Monitor (VMM) built on the seL4 Core Platform (seL4CP).
It is (at least initially) a port of the
[CAmkES VMM](https://github.com/sel4/camkes-vm-examples). See the bottom of the
README for progress/planned features.

Due to being a work-in-progress, expect frequent changes to the VMM as well as the
SDK used by the VMM.

For information on how to use the VMM, supported platforms, how to add your own
platform, etc, please see the [manual](docs/MANUAL.md).

## Getting started

The VMM is intended to be built on Linux and macOS.

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

Finally, you will need an experimental seL4 Core Platform SDK.

* Currently virtualisation support and other patches that the VMM requires are
  not part of mainline seL4 Core Platform. See below for instructions on
  acquiring the SDK.
* Upstreaming the required changes is in-progress.

#### Acquiring the SDK

For acquiring the SDK, you have two options.

1. Build the SDK yourself.
2. Download a pre-built SDK.

Option 2 is not available for now due to the SDK frequently changing.

##### Option 1 - Building the SDK

You will need a custom seL4CP SDK. You can acquire it with the following commands:
```sh
git clone https://github.com/Ivan-Velickovic/sel4cp.git --branch dev
```

Note that you will also need a slightly modified seL4 kernel. You can acquire it
with the following commands:
```sh
git clone https://github.com/Ivan-Velickovic/seL4.git --branch sel4cp-dev
```

From here, you can follow the instructions
[here](https://github.com/Ivan-Velickovic/sel4cp/tree/dev) to build the SDK.

If you have built the SDK then the path to the SDK should look something like
this: `sel4cp/release/sel4cp-sdk-1.2.6`.

Finally, we can simulate a basic system with a single Linux guest with the
following command. We want to run the `simple` example system in a `debug`
configuration for the QEMU ARM virt system.
```sh
make BUILD_DIR=build \
     SEL4CP_SDK=/path/to/sdk \
     CONFIG=debug \
     BOARD=qemu_arm_virt_hyp \
     SYSTEM=simple.system \
     run
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
