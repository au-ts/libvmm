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
        SEL4CP_SDK=${SDK_PATH}
}

build_simple_zig() {
    BOARD=$1
    CONFIG=$2
    # @ivanv Ideally we'd have Makefile and Zig output in same
    # directory structure
    EXAMPLE_DIR="${PWD}/examples/simple"
    pushd ${EXAMPLE_DIR}
    # @ivanv: for some reason Zig compiling the C VMM only
    # works with ReleaseFast optimisations, debug does not work
    # and causes a fault to happen during Linux booting. More
    # investigation required.... sigh
    zig build \
        -Dtarget="aarch64-freestanding" \
        -Dsdk=${SDK_PATH} \
        -Dboard=${BOARD} \
        -Dconfig=${CONFIG} \
        -Doptimize=ReleaseFast
    popd
}

simulate_simple_zig() {
    BOARD=$1
    CONFIG=$2
    EXAMPLE_DIR="${PWD}/examples/simple"
    BUILD_DIR="${EXAMPLE_DIR}/zig-out/bin"
    ./ci/buildroot_login.exp ${BUILD_DIR}/loader.img
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

build_simple_zig "qemu_arm_virt_hyp" "debug"
simulate_simple_zig "qemu_arm_virt_hyp" "debug"
build_simple_zig "qemu_arm_virt_hyp" "release"
simulate_simple_zig "qemu_arm_virt_hyp" "release"

build_simple_make "odroidc4_hyp" "debug"
build_simple_make "odroidc4_hyp" "release"

build_simple_zig "odroidc4_hyp" "debug"
build_simple_zig "odroidc4_hyp" "release"
