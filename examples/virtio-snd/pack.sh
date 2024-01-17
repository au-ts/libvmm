#!/usr/bin/env bash
set -e

if [ "$#" -ne 4 ]; then
    echo "usage: $0 ROOTFS_TMP_DIR INITRD INITRD_OVERLAYED ELF1 ELF2 ..."
fi

ROOTFS_TMP_DIR="$1"
INITRD="$2"
INITRD_OVERLAYED="$3"
ELFS="$4"

echo "Packing $ELFS into $INITRD"

rm -rf $ROOTFS_TMP_DIR
mkdir -p $ROOTFS_TMP_DIR/root
cp $ELFS $ROOTFS_TMP_DIR/root
({ gunzip -dc $INITRD; cd $ROOTFS_TMP_DIR && find . | cpio -o -H newc; } | gzip > $INITRD_OVERLAYED)
