#!/bin/bash

set -e

SDK_PATH=$1

# @ivanv This is weird and needs to be done better. We should probably
# just give a path to the makefile of the system file along with images
examples=("qemu_arm_virt_hyp/systems/simple.system" "qemu_arm_virt_hyp/systems/two_vms.system" "odroidc2/systems/simple.system")

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
