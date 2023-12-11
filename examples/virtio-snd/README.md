# Audio driver playground

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

When you boot the example, you will see different coloured output for each guest. The
Linux logs will be interleaving like so:
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

You will notice that we have two Buildroot login prompts, initially all input is defaulted
to the guest one (in red). To switch to input into the other guest (green), type in `@2`.
The `@` symbol is used to switch between clients, in this case the green guest is client 2.
