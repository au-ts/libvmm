# Creating a minimal Linux VM
In certain scenarios, you may want to run only a single process or a limited set of processes within a virtual machine (VM). One common use case is the creation of a driver VM, where the primary purpose of the VM is to run a UIO (Userspace I/O) driver. In such cases, most of the typical Linux configuration is unnecessary and only adds bloat. This excess increases both the memory footprint and cache usage, which can negatively impact VM performance.

By reducing this bloat, we can optimize the VM's resource consumption, improving performance and reducing overhead. To achieve this, it's recommended to begin development and testing with a standard Linux kernel image and a root filesystem (rootfs) generated by tools like Buildroot or similar. These tools provide a more complete environment, allowing for easier debugging and development during the initial phases. Once the driver VM is stable and functioning as expected, you can begin the process of minimizing the kernel image and rootfs, stripping out unnecessary components to create a more lightweight system. This section will guide you through the process of creating a minimal linux VM.

## Minimal linux kernel image
We will be using the same linux kernel image across all the driver VMs and make use of kernel modules to add specific functionality for each of the driver VMs. You will first need to create a minimal linux kernel image by editing its configuration:

To begin, you need to create a minimal Linux kernel image by customizing its configuration. You can copy the [configuration file](/examples/virtio/board/qemu_virt_aarch64/blk_driver_vm/linux_config) from an existing minimalized driver VM and modify it to include the necessary options for your driver, primarily as kernel modules.
```sh
# Clone the linux repo
git clone --depth 1 --branch v5.18 https://github.com/torvalds/linux.git

# Copy linux_config from an existing driver VM
cp linux_config linux/.config

# This will start a terminal-oriented configuration tool
make -C linux ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- menuconfig
```

Note that some kernel configurations cannot be built as modules and must be compiled directly into the kernel. In such cases, you will need to update the kernel image for all driver VMs.

Once all the required options have been selected, you can compile the kernel using:
```sh
# nproc returns the number of available processors
make -C linux ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- all -j$(nproc)
```

Copy the resulting image to your driver VM directory:
```sh
cp linux/arch/arm64/boot/Image driver_vm/linux
```

## Minimal userspace
Similar to the kernel, we aim to create a minimal root filesystem (rootfs) for the driver VM. The goal is to include only the essential components, such as the UIO driver and the necessary files and directories to support its operation, minimizing unnecessary content to reduce the overall size and complexity of the system.

### Steps to create a minimal rootfs:

1. Start by setting up the basic directory layout for the root filesystem:
```sh
mkdir -p rootfs/{dev,sys,modules}
```

2. Now, compile the kernel modules that were selected during the kernel configuration:
```sh
# This will compile the kernel modules and install them in a directory called virtio_modules inside the linux directory
make -C linux ARCH=arm64 CROSS_COMPILE=aarch64-none-elf- modules_install INSTALL_MOD_PATH=./virtio_modules
```

This step will generate the kernel modules for all the drivers, including those not needed for your specific driver VM. Additionally, other metadata files (like module loading order) will be generated, but for your purposes, only the actual modules are relevant since you will load them manually in the VM.

3. Navigate to the `linux/virtio_modules/lib/modules/5.18.0/kernel` directory and locate the specific modules required for your driver VM. For example, if you're building a block driver VM, you will need the `virtio_blk.ko` module. Copy the necessary modules to your rootfs:

```sh
cp linux/virtio_modules/lib/modules/5.18.0/kernel/drivers/block/virtio_blk.ko rootfs/modules
```

4. After copying the kernel modules, include any additional files that are essential for your driver VM to operate. These could include configuration files, or additional programs. Whenever possible, it is recommended to use statically linked binaries to avoid dependencies on shared libraries. However, if static linking is not feasible, you can copy the necessary shared libraries into the rootfs to support these programs.

5. Once all necessary files are in place, you can package the rootfs into an initramfs image, which will be loaded by the VM:

```sh
cd rootfs
find . | cpio -ov -H newc | gzip > rootfs.cpio.gz
```

## Creating an Init Process for the Driver VM
Since the root filesystem (rootfs) doesn't include a shell, you'll need to create a custom init program that runs as process ID 1 (PID 1). This program will be responsible for loading the necessary kernel modules and starting the driver. It is essential that the init program has all its dependencies statically linked to avoid requiring shared libraries within the rootfs.

To help with this, we have provided a bootstrap script, [init.c](/tools/linux/uio/init.c), which handles common initialization tasks for all driver VMs, such as mounting the `/dev` and `/sys` directories. However, to adapt this script for your specific driver VM, you'll need to implement two key functions: `load_modules` and `init_main`, both of which are defined in [init.h](/tools/linux/include/uio/init.h).

- `load_modules`: This function is responsible for loading the required kernel modules. You can use the `insmod` helper (declared in [init.h](/tools/linux/include/uio/init.h)) to load each module individually.
- `init_main`: This function will start your driver. Ideally, init_main should not return unless there is an error, meaning it will keep running the driver for the duration of the VM's operation.

After implementing the init process, you need to include it in the rootfs that you previously created. For this purpose, we have provided a script called [packrootfs](/tools/packrootfs), which automates the process of packaging the rootfs along with the init program into the final initramfs image.

Once the init program is injected into the rootfs, you will have a minimal driver VM that can be run using `libvmm`.

## Minimizing the Memory Allocated to the Driver VM
To optimize the resource usage of your driver VM, the amount of allocated memory can be reduced by decreasing the size of the guest RAM region specified in the system description file. This requires a bit of trial and error to determine the minimum memory needed for the VM to function properly. The general approach involves gradually reducing the guest RAM size and verifying that the driver VM can still boot and operate as expected.

1. Start by lowering the size of the guest RAM region in the system description file.

2. Update the memory node in the DTS (Device Tree Source) file to reflect the new size. This ensures that the linux VM is aware of the updated memory allocation.

3. In the DTS, you'll also need to modify the `initrd-start` and `initrd-end` fields in the chosen node. The `initrd-start` should be a memory address that is beyond the end of the Linux kernel image, while the `initrd-end` should accommodate the full size of the initramfs. Make sure that this range is large enough to hold the entire initramfs image.

4. After updating the DTS file, modify the parameters passed to the `linux_setup_images` and `guest_start` functions in your driver VMM. These parameters need to reflect the new memory addresses for the initrd and DTB (Device Tree Blob).

By following this process, you can gradually minimize the memory allocated to your driver VM, optimizing performance while ensuring the VM still has sufficient resources to run the kernel, and the driver.