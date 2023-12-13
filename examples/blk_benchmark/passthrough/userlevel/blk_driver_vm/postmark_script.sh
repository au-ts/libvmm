#!/bin/bash

mount /dev/mmcblk0p1 /mnt

echo 1 > /proc/sys/vm/drop_caches

./postmark.elf < postmark.conf