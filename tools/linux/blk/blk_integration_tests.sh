#!/bin/sh

# Copyright 2025, UNSW
#
# SPDX-License-Identifier: BSD-2-Clause

# Assumes a FAT FS based partition mounted at /mnt

# This file implement integration tests for libvmm's virtio-blk implementation.
# It operates on the Filesystem level instead of block level as a FS would generate
# more varied request types than a block level tool like dd can.

set -e

check_fs_errors() {
    if [ "$(dmesg | grep 'FAT-fs' | grep -vc 'Volume was not properly unmounted' )" -ne 0 ];
    then
        echo "FAILED, encountered FS errors, dmesg dump:"
        dmesg
        exit 1
    fi
}

sync_drop_slab_and_page_caches() {
    sync && echo 3 > /proc/sys/vm/drop_caches
}

TEST_ENV=/mnt/libvmm-virtio-blk-test-env

echo "Starting virtio-block integration tests"
echo "*** Preparing environment..."

if [ -e "$TEST_ENV" ];
then
    rm -rf "$TEST_ENV"
fi

sync_drop_slab_and_page_caches
if ! mkdir "$TEST_ENV";
then
    echo "FAILED to create test env folder at $TEST_ENV"
fi
if ! cd "$TEST_ENV";
then
    echo "FAILED to enter test env folder at $TEST_ENV"
fi
sync_drop_slab_and_page_caches

check_fs_errors

echo "OK"

# =============================================================
# Check R/W to unaligned sectors on the sDDF 4K transfer window
echo "*** Test 1: Create 100 folders, stress unaligned access..."

NUM_FOLDERS_EXPECTED=100

# Create
for SEQUENCE in $(seq 1 "$NUM_FOLDERS_EXPECTED");
do
    if ! mkdir "$SEQUENCE";
    then
        echo "FAILED, couldn't create folder at sequence $SEQUENCE"
        exit 1
    fi
done && sync

# Drop page caches to trigger fresh reads from blk dev
sync_drop_slab_and_page_caches

# Now read, first sanity check
# shellcheck disable=SC2012
# Don't need to use find here
NUM_FOLDERS_GOT=$(ls | wc -w)
if [ "$NUM_FOLDERS_GOT" -ne "$NUM_FOLDERS_EXPECTED" ];
then
    echo "FAILED, expected $NUM_FOLDERS_EXPECTED folders to exists but only found $NUM_FOLDERS_GOT"
    exit 1
fi

# Then check that the names aren't corrupted
SEQUENCE="1"
# shellcheck disable=SC2012
for FOLDER in $(ls | sort -n);
do
    if [ "$FOLDER" -ne "$SEQUENCE" ];
    then
        echo "FAILED, at expected folder with name $SEQUENCE but got $FOLDER"
        exit 1
    fi
    SEQUENCE=$((SEQUENCE + 1))
done

check_fs_errors

# Make sure the root point is still in a sane state
if ! ls .. >/dev/null;
then
    echo "FAILED, couldn't ls the root point"
    exit 1
fi

check_fs_errors

echo "PASSED"

# =============================================================
# Check bulk data transfer, stresses sDDF block data region full handling
echo "*** Test 2: Bulk data transfer (will take a few minutes)..."
SEQ_START=0
SEQ_END=1000000
BYTES_EXPECTED=6888897

if ! seq -s ',' "$SEQ_START" "$SEQ_END" | tr -d '\n' >test_data.txt;
then
    echo "FAILED, couldn't write data to test file"
    exit 1
fi

sync_drop_slab_and_page_caches

# Now sanity read
if ! READ_BYTES=$(wc -c <"test_data.txt");
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
for READ_NUM in $(tr ',' ' ' <"test_data.txt");
do
    if [ "$READ_NUM" -ne "$SEQUENCE" ];
    then
        echo "FAILED, at expected sequence number $SEQUENCE but got $READ_NUM"
        exit 1
    fi
    SEQUENCE=$((SEQUENCE + 1))
done

check_fs_errors
sync_drop_slab_and_page_caches
echo "PASSED"

# =============================================================
# Now check that writing a bunch of stuff didn't corrupt the filesystem
echo "*** Test 3: Root point sanity check..."

if ! ls .. >/dev/null;
then
    echo "FAILED, couldn't ls the root point"
    exit 1
fi

check_fs_errors
sync_drop_slab_and_page_caches
echo "PASSED"

# =============================================================
echo "*** Cleaning environment..."

if ! cd ..;
then
    echo "FAILED...cannot escape test dir"
    exit 1
fi
if ! rm -rf "$TEST_ENV";
then
    echo "FAILED...cannot remove test dir"
    exit 1
fi

sync_drop_slab_and_page_caches
if [ -d "$TEST_ENV" ];
then
    echo "FAILED, deleted test dir but it still exists?"
    exit 1
fi

check_fs_errors

echo "OK"
echo "All is well in the universe"
exit 0
