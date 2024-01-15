#!/usr/bin/env bash
set -e

[ -z "$ROOTFS_TMP_DIR" ]   && (echo "Missing ROOTFS_TMP_DIR" && exit 1)
[ -z "$INITRD" ]           && (echo "Missing INITRD" && exit 1)
[ -z "$INITRD_OVERLAYED" ] && (echo "Missing INITRD_OVERLAYED" && exit 1)
[ -z "$ELFS" ]             && (echo "Missing ELFS" && exit 1)

rm -rf $ROOTFS_TMP_DIR
mkdir -p $ROOTFS_TMP_DIR/root
cp $ELFS $ROOTFS_TMP_DIR/root
({ gunzip -dc $INITRD; cd $ROOTFS_TMP_DIR && find . | cpio -o -H newc; } | gzip > $INITRD_OVERLAYED)
