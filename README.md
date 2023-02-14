# VMM on the seL4 Core Platform

This is an **experimental** Virtual Machine Monitor (VMM) for 64-bit ARM platforms
built on the seL4 Core Platform (seL4CP). It is (at least initially) a port of the
[CAmkES VMM](https://github.com/sel4/camkes-vm-examples).
See the bottom of the README for progress/planned features.

Due to being a work-in-progress, expect frequent changes to the VMM as well as the SDK used by the VMM.

For information on how to use the VMM, supported platforms, how to add your own platform etc, please see the [documentation](docs/README.md).

## Building the VMM

### Dependencies

* Make
* dtc (Device tree compiler)
* AArch64 cross compiler toolchain
* QEMU (if you wish to simulate the VMM)

Using `apt`:

`sudo apt update && sudo apt install make gcc-aarch64-linux-gnu qemu-system-arm device-tree-compiler`

Also, you'll need:
* An experimental seL4 Core Platform SDK
    * Currently virtualisation support, RISC-V support and other patches that
      the VMM requires are not part of mainline seL4 Core Platform. See below
      for instructions on acquiring the SDK.

#### Acquiring the SDK

For acquiring the SDK, you have two options.

1. Build the SDK yourself.
2. Download a pre-built SDK.

Option 2 is not available for now due to the SDK frequently changing.

##### Option 1 - Building the SDK

You will need a custom seL4CP SDK. You can acquire it with the following commands:
```sh
git clone https://github.com/Ivan-Velickovic/sel4cp.git
cd sel4cp
git checkout dev
```

Note that you will also need a slightly modified seL4 kernel. You can acquire it with the following commands:
```sh
git clone https://github.com/Ivan-Velickovic/seL4.git
cd seL4
git checkout sel4cp-dev
```

From here, you can follow the instructions [here](https://github.com/Ivan-Velickovic/sel4cp/tree/dev) to build the SDK.

After acquiring this repository and all the dependencies, we can simulate a basic system with a single guest OS:
```sh
make BUILD_DIR=build SEL4CP_SDK=/path/to/sdk SEL4CP_CONFIG=debug SEL4CP_BOARD=qemu_arm_virt_hyp SYSTEM=simple.system run
```

If you have built the SDK then the path to the SDK should look something like this: `sel4cp/release/sel4cp-sdk-1.2.6`.

## Roadmap

The following is planned to be added to the VMM.

* Support QEMU ARM "virt" with GICv3
* Support QEMU ARM "virt" with other CPUs such as Cortex-A72
* Fix up all the system files, add CI to make sure they all work
* Make it easier to pass a system file and Linux images to the Makefile
* Get OdroidC4 working
* Get i.MX8MM fully working (partial support now)
* Get RPi4B+ working
* General improvements to the usability and extensibility of the VMM.
    * This means that the VMM should allow you to easily create and use multiple VMs and be able to restart them.
    * There is a lack of fragility. Configuring the VMs/VMM should not have significant friction.
* GICv4 support to allow for direct injection of vLPIs.
* SMP support. This includes having one VM using multiple cores
  as well as placing separate VMs on separate cores.
* virtIO back-ends (console, socket, network, block)
* 64-bit RISC-V support
* 64-bit x86 support

Currently, there are no plans for 32-bit guests or 32-bit hosts.
