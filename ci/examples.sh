#!/bin/bash

set -e

SDK_PATH=$1

[[ -z $SDK_PATH ]] && echo "usage: examples.sh [PATH TO SDK]" && exit 1
[[ ! -d $SDK_PATH ]] && echo "The path to SDK provided does not exist: '$SDK_PATH'" && exit 1

build_simple() {
    BOARD=$1
    CONFIG=$2
    BUILD_DIR="build_simple_${CONFIG}_${BOARD}"
    make -C examples/simple -B BUILD_DIR=${BUILD_DIR} CONFIG=${CONFIG} BOARD=${BOARD} SEL4CP_SDK=${SDK_PATH}
}

simulate_simple() {
    BOARD=$1
    CONFIG=$2
    BUILD_DIR="build_simple_${CONFIG}_${BOARD}"
    ./ci/buildroot_login.exp examples/simple/${BUILD_DIR}/loader.img
}

build_simple "qemu_arm_virt_hyp" "debug"
simulate_simple "qemu_arm_virt_hyp" "debug"
build_simple "qemu_arm_virt_hyp" "release"
simulate_simple "qemu_arm_virt_hyp" "release"

build_simple "odroidc4_hyp" "debug"
build_simple "odroidc4_hyp" "release"
