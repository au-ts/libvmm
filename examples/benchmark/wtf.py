#!/usr/bin/python3

import sys

if len(sys.argv) < 3:
    print("Usage: ./wtf.py <bare test name> <platform>")
    sys.exit(1)

baremetal_path = f"{sys.argv[1]}/build/{sys.argv[2]}/baremetal.bin"

f = open(baremetal_path, "ab")

f.write(bytes(('!' * (1024 * 1024 * 30)), 'utf-8'))

f.close()

