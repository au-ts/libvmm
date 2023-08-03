#!/bin/bash

set -e

SDK_PATH=$1

[[ -z $SDK_PATH ]] && echo "usage: examples.sh [PATH TO SDK]" && exit 1
[[ ! -d $SDK_PATH ]] && echo "The path to SDK provided does not exist: '$SDK_PATH'" && exit 1

build_simple_make() {
    BOARD=$1
    CONFIG=$2
    BUILD_DIR="${PWD}/build/simple/make/${BOARD}/${CONFIG}"
    mkdir -p ${BUILD_DIR}
    make -C examples/simple -B \
        BUILD_DIR=${BUILD_DIR} \
        CONFIG=${CONFIG} \
        BOARD=${BOARD} \
        SEL4CP_SDK=${SDK_PATH} \
        -j$(nproc)
}

simulate_simple_make() {
    BOARD=$1
    CONFIG=$2
    BUILD_DIR="${PWD}/build/simple/make/${BOARD}/${CONFIG}"
    ./ci/buildroot_login.exp ${BUILD_DIR}/loader.img
}

build_simple_make "qemu_arm_virt_hyp" "debug"
simulate_simple_make "qemu_arm_virt_hyp" "debug"
build_simple_make "qemu_arm_virt_hyp" "release"
simulate_simple_make "qemu_arm_virt_hyp" "release"

build_simple_make "odroidc4_hyp" "debug"
build_simple_make "odroidc4_hyp" "release"
