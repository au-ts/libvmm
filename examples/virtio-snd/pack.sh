#!/usr/bin/env bash
set -e

if [ "$#" -ne 3 ]; then
    echo "usage: $0 ROOTFS_TMP_DIR INITRD INITRD_OVERLAYED"
fi

ROOTFS_TMP_DIR="$1"
INITRD="$2"
INITRD_OVERLAYED="$3"

({ gunzip -dc $INITRD; cd $ROOTFS_TMP_DIR && find . | cpio -o -H newc; } | gzip > $INITRD_OVERLAYED)
