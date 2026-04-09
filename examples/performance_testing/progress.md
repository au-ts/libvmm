gdb-multiarch using the .gdbinit

break vmm.c:105

#!/bin/bash
make \
    MICROKIT_SDK=/home/alexkv/thesis/microkit-sdk-2.2.0 \
    MICROKIT_BOARD=qemu_virt_aarch64 \
    MICROKIT_CONFIG=benchmark \
    ${1:-all}