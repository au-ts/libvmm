#!/bin/bash

# Copyright 2026, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause

set -e

if [[ $# -lt 5 ]]; then
  echo "Usage: ./offline_install_guest_os.sh [Windows 11 ISO] [virtIO Drivers ISO] [unattend ISO] [disk size GB] [output dir]"
  exit
fi

INSTALL_MEDIA_ISO=$1
VIRTIO_ISO=$2
UNATTEND_ISO=$3
DISK_SIZE_GB=$4
OUTPUT_DIR=$5

if ! [[ "$DISK_SIZE_GB" =~ ^-?[0-9]+$ ]]; then
  echo "Error: DISK_SIZE_GB needs to be an integer"
  exit 1
fi

if [ "$DISK_SIZE_GB" -lt 32 ]; then
  echo "Error: DISK_SIZE_GB needs to be at least 32"
  exit 1
fi

if [ -e "/path/to/dir" ]; then
  echo "Error: Output directory already exists"
  exit 1
fi

if ! mkdir "$OUTPUT_DIR"; then
  echo "Error: Failed to create output directory"
  exit 1
fi

RAW_DISK=$(realpath "$OUTPUT_DIR/guest_disk.raw")
if ! truncate -s "${DISK_SIZE_GB}G" "$RAW_DISK"; then
  echo "Error: Failed to create intermediate disk image"
  exit 1
fi

OUTPUT_DISK_SIZE_GB=$((DISK_SIZE_GB + 1))
OUT_DISK=$(realpath "$OUTPUT_DIR/guest_disk.libvmm")
if ! truncate -s "${OUTPUT_DISK_SIZE_GB}G" "$OUT_DISK"; then
  echo "Error: Failed to create final disk image"
  exit 1
fi

# -display sdl give greater performance compared to the default which is GTK
if ! qemu-system-x86_64 \
      -display sdl \
      -vga none \
      -device bochs-display,xres=1920,yres=1080 \
      -accel kvm \
      -smp 4 \
      -m 8G \
      -cpu host \
      -drive file="$RAW_DISK",if=virtio,format=raw \
      -global virtio-blk-pci.disable-legacy=on \
      -drive file="$INSTALL_MEDIA_ISO",media=cdrom \
      -drive file="$VIRTIO_ISO",media=cdrom \
      -drive file="$UNATTEND_ISO",media=cdrom \
      -bios /usr/share/ovmf/x64/OVMF.4m.fd; then
  echo "Error: Failed to launch QEMU"
  exit 1
fi

sfdisk --no-reread --no-tell-kernel "$OUT_DISK" <<EOF
label: dos

start=2048,size="${DISK_SIZE_GB}G"
EOF

dd if="$RAW_DISK" of="$OUT_DISK" bs=4096 conv=notrunc,sync,sparse seek=256 status=progress

echo "ALL DONE"
exit 0