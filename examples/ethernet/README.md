# Pass-through ethernet device example

This example shows a VMM booting a Linux kernel, where as a guest it is access
to the ethernet device for the platform along with the driver for it.

The example currently works on the following platforms:
* QEMU ARM virt

## Building with Make

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

