#!/bin/bash

# Copyright 2025, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause

set -e

IMAGE_PATH=$1
IMAGE_NAME=$2
METADATA_PATH=$3

[[ -z "$IMAGE_PATH" || -z "$IMAGE_NAME" ]] && echo "usage: package_image.sh image_path package_name [image_metadata_path]" && exit 1

SHASUM=$(shasum "$IMAGE_PATH" | cut -d' ' -f1)

# Clean the image name if it contain dots, a common case is `rootfs.cpio.gz`
# Because it can interfere with the Zig syntax
SANITISED_IMG_NAME=$(echo "$IMAGE_NAME" | tr '.' '_')

DIR_NAME="$SHASUM-$IMAGE_NAME"
TAR_NAME="$DIR_NAME.tar"

mkdir "$DIR_NAME"
cp "$IMAGE_PATH" "$DIR_NAME"
if [ "$METADATA_PATH" ];
then
    cp "$METADATA_PATH" "$DIR_NAME"
fi

tar cf "$TAR_NAME" "$DIR_NAME"
gzip "$TAR_NAME"
