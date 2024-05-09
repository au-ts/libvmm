# A simple VMM for running Linux guests

This example is a minimal VMM that supports Linux guests and a basic
buildroot/BusyBox root file system. This gives a basic command-line with some
common Linux utilities.

The example currently works on the following platforms:
* QEMU ARM virt
* HardKernel Odroid-C4

## Building with Make

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

## Building with Zig

For educational purposes, you can also build and run this example using the
[Zig](https://ziglang.org/) build system.

At the moment, Zig still under heavy development and hence this example depends
on the 'master' version of Zig for now. This example has been built using
`0.12.0-dev.3434+e90583f5d`, so anything equal to or above that version should work.

You can download Zig [here](https://ziglang.org/download/).

```sh
zig build -Dsdk=/path/to/sdk -Dboard=<BOARD>
```

Where `<BOARD>` is one of:
* `qemu_arm_virt`
* `odroidc4`

If you are building for QEMU then you can also run QEMU by doing:
```sh
zig build -Dsdk=/path/to/sdk -Dboard=qemu_arm_virt qemu
```

You can view other options by doing:
```sh
zig build -Dsdk=/path/to/sdk -Dboard=<BOARD> -h
```

