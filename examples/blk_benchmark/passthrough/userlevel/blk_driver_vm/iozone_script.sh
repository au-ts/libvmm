#!/bin/bash

mount /dev/mmcblk0p1 /mnt

echo 1 > /proc/sys/vm/drop_caches

touch /mnt/test

rm output
for run in 1 2 3
do
    for i in 4 16 512 1024 16384
    do
        iozone -f/mnt/test -o -s65536 -r$i -i0 -i1 >> output$run
    done
done