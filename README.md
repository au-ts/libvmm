# AArch64 VMM on the seL4 Core Platform

This is an expiremental VMM for 64-bit ARM platforms built on the seL4 Core Platform (seL4CP). It is (at least initially) a port of the [CAmkES VMM](https://github.com/sel4/camkes-vm-examples). The current goal is to have a Linux guest boot and run in order start evaluating the virtualisation extensions to seL4CP.

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
