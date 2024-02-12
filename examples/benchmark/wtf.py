#!/usr/bin/python3

import sys

if len(sys.argv) < 3:
    print("Usage: ./wtf.py <bare test name> <platform>")
    sys.exit(1)


test = sys.argv[1]
platform = sys.argv[2]

if sys.argv[1] == "boot":
    platform += "/128k"

baremetal_path = f"{test}/build/{platform}/baremetal.bin"
f = open(baremetal_path, "ab")

f.write(bytes(('!' * (1024 * 1024 * 30)), 'utf-8'))

f.close()


