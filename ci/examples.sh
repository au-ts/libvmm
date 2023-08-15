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
    zig build \
        -Dsdk=${SDK_PATH} \
        -Dboard=${BOARD} \
        -Dconfig=${CONFIG}
    popd
}

build_rust() {
    CONFIG=$1
    BUILD_DIR="${PWD}/examples/rust/build"
    mkdir -p ${BUILD_DIR}
    make -C examples/rust -B \
        CONFIG=${CONFIG} \
        SEL4CP_SDK=${SDK_PATH}
}

simulate_rust() {
    BUILD_DIR="${PWD}/examples/rust/build"
    ./ci/buildroot_login.exp ${BUILD_DIR}/loader.img
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

echo "Passed all VMM tests"
