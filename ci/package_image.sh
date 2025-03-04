#!/bin/bash

# Copyright 2025, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause

set -e

IMAGE_PATH=$1
IMAGE_NAME=$2

[[ -z $IMAGE_PATH || -z $IMAGE_NAME ]] && echo "usage: package_image_zig.sh [PATH TO IMAGE] [NAME OF IMAGE]" && exit 1

# Zig package manager expects a build.zig.zon alongside the artefact, we also need
# to zip/tar it.

ZON_PATH="build.zig.zon"

SHASUM=$(shasum $IMAGE_PATH | cut -d' ' -f1)

# Clean the image name if it contain dots, a common case is `rootfs.cpio.gz`
# Because it can interfere with the Zig syntax
SANITISED_IMG_NAME=$(echo "$IMAGE_NAME" | tr '.' '_')

cat > $ZON_PATH <<EOF
.{
    .name = .$SANITISED_IMG_NAME,
    .version = "0.0.0",

    .paths = .{
        "$IMAGE_NAME",
    }
}
EOF

DIR_NAME="$SHASUM-$IMAGE_NAME"
TAR_NAME="$DIR_NAME.tar"

mkdir "$DIR_NAME"
cp $ZON_PATH $DIR_NAME
cp $IMAGE_PATH $DIR_NAME

tar cf $TAR_NAME $DIR_NAME
gzip $TAR_NAME
