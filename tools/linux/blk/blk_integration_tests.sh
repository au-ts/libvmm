#!/bin/sh

# Copyright 2025, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause

# Assumes a FAT FS based partition mounted at /mnt

# This file implement integration tests for libvmm's virtio-blk implementation.
# It operates on the Filesystem level instead of block level as a FS would generate
# more varied request types than a block level tool like dd can.

# IMPORTANT: it is recommended that you run this script twice.

set -e

TEST_ENV=/mnt/libvmm-virtio-blk-test-env

echo "Starting virtio-block integration tests"

echo -n "*** Preparing environment..."

if [ -e "$TEST_ENV" ];
then
    rm -rf "$TEST_ENV"
fi && sync

mkdir "$TEST_ENV" && cd "$TEST_ENV" && sync && echo 3 > /proc/sys/vm/drop_caches

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
# Check R/W to unaligned sectors on the sDDF 4K transfer window
echo -n "*** Test 1: Create 100 folders, stress unaligned access..."

# Create
for i in $(seq 1 100);
do 
    mkdir "$i"
done && sync

if [ "$?" -ne 0 ];
then
    echo "FAILED, couldn't create some folders"
    exit 1
fi

# Drop page caches to trigger fresh reads from blk dev
sync && echo 3 > /proc/sys/vm/drop_caches

# Now read, first sanity check
NUM_FOLDERS=$(ls | wc -w)
if [ "$NUM_FOLDERS" -ne 100 ];
then
    echo "FAILED, expected 100 folders to exists but only found $NUM_FOLDERS"
    exit 1
fi

# Then check that the names aren't corrupted
SEQUENCE="1"
for FOLDER in $(ls | sort -n);
do
    if [ "$FOLDER" -ne "$SEQUENCE" ];
    then
        echo "FAILED, at expected folder with name $SEQUENCE but got $FOLDER"
        exit 1
    fi
    SEQUENCE=$((SEQUENCE + 1))
done

if [ $(dmesg | grep "FAT-fs" | grep -v "Volume was not properly unmounted" | wc -l) -ne 0 ];
then
    echo "FAILED, encountered FS errors, dmesg dump:"
    dmesg
    exit 1
fi

echo "PASSED"

# =============================================================
# Check bulk data transfer, stresses sDDF block data region full handling
echo -n "*** Test 2: Bulk data transfer (will take a few minutes)..."
NUMS=1000000
BYTES_EXPECTED=6888897

seq -s ',' 0 "$NUMS" | tr -d '\n' >test_data.txt
if [ "$?" -ne 0 ];
then
    echo "FAILED, couldn't write data to test file"
    exit 1
fi

sync && echo 3 > /proc/sys/vm/drop_caches

# Now sanity read
READ_BYTES=$(cat "test_data.txt" | wc -c)
if [ "$?" -ne 0 ];
then
    echo "FAILED, couldn't read back data from test file"
    exit 1
fi
if [ "$READ_BYTES" != "$BYTES_EXPECTED" ];
then
    echo "FAILED, expected $BYTES_EXPECTED bytes but only got $READ_BYTES"
    exit 1
fi

# Check that each byte is equal to what we written out
# Will take a few minutes.
SEQUENCE="0"
for READ_NUM in $(cat "test_data.txt" | tr ',' ' ');
do
    if [ "$READ_NUM" -ne "$SEQUENCE" ];
    then
        echo "FAILED, at expected sequence number $SEQUENCE but got $READ_NUM"
        exit 1
    fi
    SEQUENCE=$((SEQUENCE + 1))
done

if [ $(dmesg | grep "FAT-fs" | grep -v "Volume was not properly unmounted" | wc -l) -ne 0 ];
then
    echo "FAILED, encountered FS errors, dmesg dump:"
    dmesg
    exit 1
fi

sync && echo 3 > /proc/sys/vm/drop_caches
echo "PASSED"

# =============================================================
echo -n "*** Cleaning environment..."

cd ~ && rm -rf "$TEST_ENV" && sync
if [ "$?" -ne 0 ];
then
    echo "FAILED...cannot remove test dir"
    exit 1
fi

echo 3 > /proc/sys/vm/drop_caches
if [ -d "$TEST_ENV" ];
then
    echo "FAILED, deleted test dir but it still exists?"
    exit 1
fi

if [ $(dmesg | grep "FAT-fs" | grep -v "Volume was not properly unmounted" | wc -l) -ne 0 ];
then
    echo "FAILED, encountered FS errors, dmesg dump:"
    dmesg
    exit 1
fi

echo "OK"
echo "All is well in the universe"
exit 0
