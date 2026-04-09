#!/bin/bash
make \
    MICROKIT_SDK=/home/alexkv/thesis/microkit-sdk-2.0.1 \
    MICROKIT_BOARD=qemu_virt_aarch64 \
    "$@" qemu
