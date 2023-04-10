#!/bin/bash

set -e

SDK_PATH=$1

[ ! -d $SDK_PATH ] && echo "The path to SDK provided does not exist: $SDK_PATH" && exit

EXAMPLES=("qemu_arm_virt_hyp/systems/simple.system")
CONFIGS=("debug release")

for EXAMPLE in ${EXAMPLES[@]}; do
    for CONFIG in ${CONFIGS[@]}; do
        echo "Building example: $EXAMPLE in config: $CONFIG"
        BOARD=$(echo "$EXAMPLE" | cut -d "/" -f1)
        SYSTEM=$(echo "$EXAMPLE" | cut -d "/" -f3)
        SYSTEM_NAME=$(basename $SYSTEM .system)
        BUILD_DIR="build_${SYSTEM_NAME}_${BOARD}_${CONFIG}"

        make -j$(nproc) -B \
            BUILD_DIR=$BUILD_DIR \
            SEL4CP_SDK=$SDK_PATH \
            CONFIG=$CONFIG \
            BOARD=$BOARD \
            SYSTEM=$SYSTEM
    done
done

for EXAMPLE in ${EXAMPLES[@]}; do
    for CONFIG in ${CONFIGS[@]}; do
        BOARD=$(echo "$EXAMPLE" | cut -d "/" -f1)
        BUILD_DIR="build_${SYSTEM_NAME}_${BOARD}"
        SYSTEM=$(echo "$EXAMPLE" | cut -d "/" -f3)
        SYSTEM_NAME=$(basename $SYSTEM .system)
        BUILD_DIR="build_${SYSTEM_NAME}_${BOARD}_${CONFIG}"
        # For now we will only do runtime tests via simulation
        if [[ $BOARD == "qemu_arm_virt_hyp" ]]; then
            echo "Running EXAMPLE: $EXAMPLE"
            IMAGE="$BUILD_DIR/loader.img"
            ./ci/buildroot_login.exp $IMAGE
        fi
    done
done
