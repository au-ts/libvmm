#!/bin/bash
make \
    MICROKIT_SDK=/home/alexkv/thesis/microkit-sdk-2.2.0 \
    MICROKIT_BOARD=qemu_virt_aarch64 \
    MICROKIT_CONFIG=benchmark \
    ${1:-all}

# MICROKIT_BOARD=odroidc4 \ qemu_virt_aarch64