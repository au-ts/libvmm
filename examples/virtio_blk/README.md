# Using virtIO block with driver VM

This example shows off the virtIO support that libvmm provides using the
[seL4 Device Driver Framework](https://github.com/au-ts/sddf) to talk to a linux block driver VM that has passthrough access to the actual hardware.

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

This example consist of a guest linux client VM that talks virtIO with a backend that consists of a driver VM that has passthrough access to the block device. For debugging purposes, both client and driver VMs also use virtIO serial connected to a serial system in a similar set up to the [virtio](https://github.com/au-ts/libvmm/blob/main/examples/virtio/README.md) example.

When you boot the example, you will see different coloured output for each guest. The block driver VM in red will boot first. When it is ready, the client VM in green will boot.

After the block client VM boots it will attempt to mount the virtIO block device `/dev/vda` into `/mnt`. The block device contains an ext4 filesystem allowing us to test it using block benchmark programs like `postmark`, which has been included in `/root`.

Client VM requests to the virtIO block device will print out debugging messages.
```
Starting syslogd: OK
Starting klogd: OK
Running sysctl: OK
Saving random seed: [   18.959300] random: crng init done
OK
Starting network: OK
VIRTIO(BLOCK): ------------- Driver notified device -------------
VIRTIO(BLOCK): ----- Request type is 0x0 -----
VIRTIO(BLOCK): Request type is VIRTIO_BLK_T_IN
VIRTIO(BLOCK): Sector (read/write offset) is 2
VIRTIO(BLOCK): Descriptor index is 1, Descriptor flags are: 0x3, length is 0x400
UIO_DRIVER: received irq, count: 2
UIO_DRIVER(BLOCK): Received command: code=0, addr=0x30601000, block_number=1, count=1, id=1
VIRTIO(BLOCK): ------------- Driver notified device -------------
VIRTIO(BLOCK): ----- Request type is 0x0 -----
VIRTIO(BLOCK): Request type is VIRTIO_BLK_T_IN
VIRTIO(BLOCK): Sector (read/write offset) is 0
VIRTIO(BLOCK): Descriptor index is 1, Descriptor flags are: 0x3, length is 0x1000
UIO_DRIVER: received irq, count: 3
UIO_DRIVER(BLOCK): Received command: code=0, addr=0x30601400, block_number=0, count=4, id=2
...
...
...
[   19.708718] EXT4-fs (vda): mounted filesystem without journal. Quota mode: none.
```

Similar to the [virtio](https://github.com/au-ts/libvmm/blob/main/examples/virtio/README.md) example, initially all input is defaulted to the client VM (in green). To switch input to the driver VM (in red), type in `@2`. The `@` symbol is used to switch between clients of the serial system.


