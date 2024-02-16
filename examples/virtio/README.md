# Using virtIO with multiple Linux guests

This example shows off the virtIO support that libvmm provides using the
[seL4 Device Driver Framework](https://github.com/au-ts/sddf) to talk to the
actual hardware.

The example currently works on the following platforms:
* QEMU ARM virt

## Building

```sh
make BOARD=<BOARD> MICROKIT_SDK=/path/to/sdk
```

Where `<BOARD>` is one of:
* `qemu_arm_virt`
* `odroidc4`

Other configuration options can be passed to the Makefile such as `CONFIG`
and `BUILD_DIR`, see the Makefile for details.

If you would like to simulate the QEMU board you can run the following command:
```sh
make BOARD=qemu_arm_virt MICROKIT_SDK=/path/to/sdk qemu
```

This will build the example code as well as run the QEMU command to simulate a
system running the whole system.

## Running

### virtIO console

This example makes use of the virtIO console device so that neither guest has access
to any serial device on the platform. The virtIO console support in libvmm talks to
a serial multiplexor which then talks to a driver for input/output to the physical
serial device.

When you boot the example, you will see different coloured output for each guest. The Linux logs will be interleaving like so:
```
Starting klogd: OKStarting klogd: 
OK
Running sysctl: Running sysctl: OK
OKSaving random seed: 
Saving random seed: [    4.070358] random: crng init done
[    4.103992] random: crng init done
OK
Starting network: OK
Starting network: OK
OK

Welcome to Buildroot
buildroot login: 
Welcome to Buildroot
buildroot login:
```

Initially all input is defaulted
to guest 1 in green. To switch to input into the other guest (red), type in `@2`.
The `@` symbol is used to switch between clients of the serial system, in this case the red guest is client 2.

### virtIO block

Guest 1 and guest 2 also doubles as a client in the block system that talks virtIO to guest 3 that acts as both a block MUX and driver VM that has passthrough access to the block device.

When you boot the example, the block driver VM will boot first. When it is ready, the client VMs will boot together.

After the client VMs boot, they will attempt to mount the first partition of the virtIO block device `/dev/vda1` into `/mnt`. This partition contains an ext4 file system. 

```
[   22.860300] EXT4-fs (vda): mounted filesystem without journal. Quota mode: none.
[   22.793574] EXT4-fs (vda): mounted filesystem without journal. Quota mode: none.
```

We can further test the virtIO block implementation by using block benchmark programs like `postmark`, which has been included in `/root`, by running it in the `/mnt` directory.

When running on the odroidc4, the system expects to read and write from the SD card, not the eMMC. When running on QEMU, read and writes go to ramdisk instead of a persistent storage device.