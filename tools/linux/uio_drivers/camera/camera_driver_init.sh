#!/bin/sh

DRIVER="uio_camera_driver"

echo $DRIVER
if [ "$1" = "init" ]; then
    echo "libcamera-hello"
    libcamera-hello 2>&1 | grep -v '#[0-9]'
    echo "gcc -Wall -Wextra -O0 $DRIVER.c -o $DRIVER"
    gcc -Wall -Wextra -O0 $DRIVER.c -o $DRIVER
fi
echo "sudo ./$DRIVER"
sudo ./$DRIVER