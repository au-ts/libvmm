#!/usr/bin/python3

# Fix for the current bad bug of small guest images breaking libvmm
# This script appends 30MB of data to the baremetal.bin file

# Used through the MAKEFILE by passing WTF_PATCH=yes to the make command

import sys
import os

if len(sys.argv) < 3:
    print("Usage: ./wtf.py <bare test name> <platform>")
    sys.exit(1)


test = sys.argv[1]
platform = sys.argv[2]

if sys.argv[1] == "boot":
    platform += "/128k"

baremetal_path = f"{test}/build/{platform}/baremetal.bin"
f = open(baremetal_path, "ab")

if os.stat(baremetal_path).st_size < (1024 * 1024 * 30):
    f.write(bytes(('!' * (1024 * 1024 * 30)), 'utf-8'))

f.close()


