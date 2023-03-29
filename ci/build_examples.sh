#!/bin/bash

set -e

SDK_PATH=$1
QEMU=qemu-system-aarch64

if ! command -v qemu-system-aarch64 &> /dev/null
then
    echo "qemu-system-aarch64 could not be found, it must be available to run the script."
    exit
fi

[ ! -d $SDK_PATH ] && echo "The path to SDK provided does not exist: $SDK_PATH" && exit

# @ivanv This is weird and needs to be done better. We should probably
# just give a path to the makefile of the system file along with images
examples=("qemu_arm_virt_hyp/systems/simple.system")

for example in ${examples[@]}; do
    echo "Building example: $example"
    BOARD=$(echo "$example" | cut -d "/" -f1)
    SYSTEM=$(echo "$example" | cut -d "/" -f3)
    SYSTEM_NAME=$(basename $SYSTEM .system)
    BUILD_DIR="build_${SYSTEM_NAME}_${BOARD}"

    make -j$(nproc) -B \
        BUILD_DIR=$BUILD_DIR \
        SEL4CP_SDK=$SDK_PATH \
        SEL4CP_CONFIG=debug \
        SEL4CP_BOARD=$BOARD \
        SYSTEM=$SYSTEM
done

for example in ${examples[@]}; do
    BOARD=$(echo "$example" | cut -d "/" -f1)
    BUILD_DIR="build_${SYSTEM_NAME}_${BOARD}"
    SYSTEM=$(echo "$example" | cut -d "/" -f3)
    SYSTEM_NAME=$(basename $SYSTEM .system)
    BUILD_DIR="build_${SYSTEM_NAME}_${BOARD}"
    # For now we will only do runtime tests via simulation
    if [[ $BOARD == "qemu_arm_virt_hyp" ]]; then
        echo "Running example: $example"
        IMAGE="$BUILD_DIR/loader.img"
        ./ci/buildroot_login.exp $IMAGE
    fi
done
