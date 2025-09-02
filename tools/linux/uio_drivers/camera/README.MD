# Camera UIO Driver for Raspberry Pi 4B

This directory contains the UIO (Userspace I/O) camera driver components for the Raspberry Pi 4B, used in conjunction with the libvmm camera example.

## Files

### uio_camera_driver.c
- This is the driver helper that accesses the control and frame memory regions through MMAPs (LibUIO).
- Configure camera set and defaults functions are broken (as incomplete).
- Example client only ever does the default setup (setup during `init_device`). If the client requests reconfiguration, the driver will not be able to implement this (not implemented due to time constraints, as this most likely requires a full media controller API understanding + usage).

### camera_driver_init.sh
- Given an `init` flag, this script will run a libcamera command to set up the media controller pipeline (needs to be done at least once every time the RPi4B is booted). It will also build the script with the correct optimization version (the least) and then runs the program.
- Invoke this script inside of RPi Linux as the build needs `<linux/videodev2.h>`.

### driver_interface.h
- Unused, but is a copy-paste of the Linux video dev 2 header file, to be used in formalizing the control region.

## Usage Instructions

1. From `libvmm/tools/linux/uio_drivers/camera`, SCP `camera_driver_init` and `uio_camera_driver.c` to the RPi4B.
2. Run `./camera_driver_init init` in the RPi4B terminal. The `init` flag needs to be used on the initial run, as it runs libcamera to set up the media controller required for V4L2, as well as compiles the driver helper.