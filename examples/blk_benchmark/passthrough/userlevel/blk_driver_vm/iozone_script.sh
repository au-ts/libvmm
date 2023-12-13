#!/bin/bash

mount /dev/mmcblk0p1 /mnt

echo 1 > /proc/sys/vm/drop_caches

touch /mnt/test

for i in 4 16 512 1024 16384
do
    iozone -f/mnt/test -o -s131072 -r$i -i0 -i1
done
