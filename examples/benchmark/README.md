# Benchmarking!

This is a benchmarking example utilising the various SPH benchmarks done by Martins and Pinto in the paper titled:
'Shedding Light on Static Partitioning Hypervisors for Arm-based Mixed-Criticality Systems'



The example currently works on the following platforms:
* HardKernel Odroid-C4
* QEMU ARM virt

## Building with Make

```sh
make BOARD=<BOARD> MICROKIT_SDK=/path/to/sdk TEST=<TEST>
```

Where `<BOARD>` is one of:
* `qemu_arm_virt`
* `odroidc4`
* `zcu102`

Where `<TEST>` is one of:
* `mibench`
* `mibench+interf`
* `irqlat`
* `irqlat+interf`

Other configuration options can be passed to the Makefile such as `CONFIG`
and `BUILD_DIR`, see the Makefile for details.

If you would like to simulate the QEMU board you can run the following command:
```sh
make BOARD=qemu_arm_virt MICROKIT_SDK=/path/to/sdk TEST=<TEST> qemu
```

This will build the benchmark code as well as run the QEMU command to simulate a
system running the whole system.



