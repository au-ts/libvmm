#!/bin/bash
make \
    MICROKIT_SDK=/home/alexkv/thesis/microkit-sdk-2.2.0-dev \
    MICROKIT_BOARD=maaxboard \
    MICROKIT_CONFIG=debug \
    ${1:-all}

# MICROKIT_BOARD=odroidc4 \ qemu_virt_aarch64