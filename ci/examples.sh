#!/bin/bash

set -e

SDK_PATH=$1

[[ -z $SDK_PATH ]] && echo "usage: examples.sh [PATH TO SDK]" && exit 1
[[ ! -d $SDK_PATH ]] && echo "The path to SDK provided does not exist: '$SDK_PATH'" && exit 1

build_simple_make() {
    BOARD=$1
    CONFIG=$2
    echo "CI|INFO: building simple example via Make with board: $BOARD and config: $CONFIG"
    BUILD_DIR="${PWD}/build/examples/simple/make/${BOARD}/${CONFIG}"
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
    echo "CI|INFO: building simple example via Zig with board: $BOARD and config: $CONFIG"
    BUILD_DIR="${PWD}/build/examples/simple/zig/${BOARD}/${CONFIG}"
    EXAMPLE_DIR="${PWD}/examples/simple"
    pushd ${EXAMPLE_DIR}
    zig build \
        -Dsdk=${SDK_PATH} \
        -Dboard=${BOARD} \
        -Dconfig=${CONFIG} \
        -p ${BUILD_DIR}
    popd
}

build_rust() {
    echo "CI|INFO: building Rust example with config: $1"
    CONFIG=$1
    BUILD_DIR="${PWD}/build/examples/rust/qemu_arm_virt/${CONFIG}"
    mkdir -p ${BUILD_DIR}
    make -C examples/rust -B \
        BUILD_DIR=${BUILD_DIR} \
        CONFIG=${CONFIG} \
        SEL4CP_SDK=${SDK_PATH}
}

simulate_rust() {
    echo "CI|INFO: simulating Rust example with config: $1"
    BUILD_DIR="${PWD}/build/examples/rust/qemu_arm_virt/${CONFIG}"
    ./ci/buildroot_login.exp ${BUILD_DIR}/loader.img
}

simulate_simple_zig() {
    BOARD=$1
    CONFIG=$2
    echo "CI|INFO: simulating simple example via Zig with board: $BOARD and config: $CONFIG"
    BUILD_DIR="${PWD}/build/examples/simple/zig/${BOARD}/${CONFIG}"
    ./ci/buildroot_login.exp ${BUILD_DIR}/bin/loader.img
}

simulate_simple_make() {
    BOARD=$1
    CONFIG=$2
    echo "CI|INFO: simulating simple example via Make with board: $BOARD and config: $CONFIG"
    BUILD_DIR="${PWD}/build/examples/simple/make/${BOARD}/${CONFIG}"
    ./ci/buildroot_login.exp ${BUILD_DIR}/loader.img
}

build_simple_make "qemu_arm_virt" "debug"
simulate_simple_make "qemu_arm_virt" "debug"
build_simple_make "qemu_arm_virt" "release"
simulate_simple_make "qemu_arm_virt" "release"

build_simple_zig "qemu_arm_virt" "debug"
simulate_simple_zig "qemu_arm_virt" "debug"
build_simple_zig "qemu_arm_virt" "release"
simulate_simple_zig "qemu_arm_virt" "release"

build_simple_make "odroidc4" "debug"
build_simple_make "odroidc4" "release"

build_simple_zig "odroidc4" "debug"
build_simple_zig "odroidc4" "release"

build_rust "debug"
simulate_rust "debug"
# @ivanv: TODO get Rust in with release version of seL4CP working
# build_rust "release"
# simulate_rust "release"

echo ""
echo "CI|INFO: Passed all VMM tests"
