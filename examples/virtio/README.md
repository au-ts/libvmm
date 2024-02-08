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

When you boot the example, you will see different coloured output for each guest.

Initially all input is defaulted
to guest 1 in green. To switch to input into the other guest (red), type in `@2`.
The `@` symbol is used to switch between clients of the serial system, in this case the red guest is client 2.

### virtIO block

Guest 1 also doubles as a client in the block system that talks virtIO to a driver VM (guest 2) that has passthrough access to the block device.

When you boot the example, the block driver VM in red will boot first. When it is ready, the client VM in green will boot.

After the block client VM boots it will attempt to mount the virtIO block device `/dev/vda` into `/mnt`. The block device contains an ext4 filesystem allowing us to test it using block benchmark programs like `postmark`, which has been included in `/root`.

Client VM requests to the driver VM will print out debugging messages using the serial system.
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
