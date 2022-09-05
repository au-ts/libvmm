# AArch64 VMM on the seL4 Core Platform

This is an **expiremental** VMM for 64-bit ARM platforms built on the seL4 Core Platform (seL4CP). It is (at least initially) a port of the [CAmkES VMM](https://github.com/sel4/camkes-vm-examples). The current goal is to have a Linux guest boot and run in order start evaluating the virtualisation extensions to seL4CP.

Due to being a work-in-progress, expect frequent changes to the VMM as well as the virtualisation extension to seL4CP.

Note that currently the VMM only runs on QEMU with the "virt" platform.

## Building the SDK

You will need a custom seL4CP SDK. You can acquire it with the following commands:
```sh
git clone https://github.com/Ivan-Velickovic/sel4cp.git
cd sel4cp
git checkout virtualisation_support
```

Note that you will also need a slightly modified seL4 kernel. You can acquire it with the following commands:
```sh
git clone https://github.com/Ivan-Velickovic/seL4.git
cd seL4
git checkout sel4cp-core-support-gen-config
```

From here, you can follow the instructions [here](https://github.com/BreakawayConsulting/sel4cp) to build the SDK.

## Building the VMM

### Dependencies

In addition to the SDK, you will need:

* dtc (Device tree compiler)
* AArch64 cross compiler toolchain
    * While any AArch64 toolchain should work, the VMM has been developed only using `aarch64-none-elf` version 10.2-2020.11, there are instructions to acquire it in the SDK's README.

After acquiring this repository, run the following command:
```sh
mkdir build
make BUILD_DIR=build SEL4CP_SDK=/path/to/sdk SEL4CP_CONFIG=debug SEL4CP_BOARD=qemu_arm_virt
```

## Running the VMM

After building the VMM, run it using the following command:
```
qemu-system-aarch64 -machine virt,virtualization=on,highmem=off,secure=off -cpu cortex-a53 -serial mon:stdio -device loader,file=build/loader.img,addr=0x70000000,cpu-num=0 -m size=2G -display none
```

## Roadmap

The following are features planned to be added to the VMM.

* GICv4 support to allow for direct injection of vLPIs.
* SMP support. This includes having one VM using multiple cores
  as well as placing seperate VMs on seperate cores.
* virtIO backends (console, socket, network, block)
* 64-bit RISC-V support
* 64-bit x86 support

Currently, there are no plans for 32-bit guests or 32-bit hosts.
