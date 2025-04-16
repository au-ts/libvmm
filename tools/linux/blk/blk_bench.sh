#!/bin/sh

# Copyright 2025, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause

# Assumes a FAT FS based partition mounted at /mnt

set -e

TEST_ENV=/mnt/libvmm-virtio-blk-bench-env

echo "Starting virtio-block bench"

echo -n "*** Preparing environment..."

if [ -e "$TEST_ENV" ];
then
    rm -rf "$TEST_ENV"
fi && sync

mkdir "$TEST_ENV" && cd "$TEST_ENV" && sync

if [ $? -ne 0 ];
then
    echo "FAILED to create test env folder at $TEST_ENV"
    exit 1
fi
if [ $(dmesg | grep "FAT-fs" | grep -v "Volume was not properly unmounted" | wc -l) -ne 0 ];
then
    echo "FAILED, encountered FS errors, dmesg dump:"
    dmesg
    exit 1
fi
echo "OK"

# =============================================================
# Now write test data
echo 3 > /proc/sys/vm/drop_caches

BLOCK_SZ=500
COUNT=100000
BYTES=$(($BLOCK_SZ * $COUNT))
echo "*** Writing $BYTES bytes..."
time sh -c "dd if=/dev/urandom of=data.txt bs=$BLOCK_SZ count=$COUNT && sync"
echo 3 > /proc/sys/vm/drop_caches

# =============================================================
# Now read it back
echo "*** Reading $BYTES_EXPECTED bytes..."
time sh -c "dd if=data.txt of=/dev/null bs=$BLOCK_SZ count=$COUNT"

echo "*** Cleaning environment..."
cd ~ && rm -rf "$TEST_ENV" && sync
if [ "$?" -ne 0 ];
then
    echo "FAILED...cannot remove test dir"
    exit 1
fi
echo "ALL DONE"
