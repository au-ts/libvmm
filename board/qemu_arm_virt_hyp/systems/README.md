<!--
     Copyright 2023, UNSW (ABN 57 195 873 179)

     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Intro

Simple documentation about the virtio demo systems. The system contains two VMMs and are connected by a vswitch.

# Usage

```
make BUILD_DIR=your/build/dir SEL4CP_SDK=your/sdk/dir SYSTEM=virtio_demo.system SEL4CP_CONFIG=debug SEL4CP_BOARD=qemu_arm_virt_hyp run
```

# Testing virtIO vswitch

command:
```
# on vm1
ifconfig eth0 192.168.1.2
ping 192.168.1.1
```

sample output:
```
# ping 192.168.1.1
PING 192.168.1.1 (192.168.1.1): 56 data bytes
64 bytes from 192.168.1.1: seq=0 ttl=64 time=17.897 ms
64 bytes from 192.168.1.1: seq=1 ttl=64 time=2.995 ms
64 bytes from 192.168.1.1: seq=2 ttl=64 time=3.347 ms
64 bytes from 192.168.1.1: seq=3 ttl=64 time=2.258 ms
64 bytes from 192.168.1.1: seq=4 ttl=64 time=2.982 ms
64 bytes from 192.168.1.1: seq=5 ttl=64 time=2.735 ms
^C
--- 192.168.1.1 ping statistics ---
6 packets transmitted, 6 packets received, 0% packet loss
round-trip min/avg/max = 2.258/5.369/17.897 ms
```

# Issue

1. We don't have a serial multiplexor, so only one VMM get to have the passthrough serial device and be able to do serial I/O

2. For some reason we need to increase the qemu memory to 4096 in the sel4cp SDK in order to get the system run:
    ```
    --- a/build_sdk.py
    +++ b/build_sdk.py
    @@ -185,7 +185,8 @@ SUPPORTED_BOARDS = (
                # Surely there would be enough space? But we can't find an untyped
                # that's at least the 512MiB in size. Come back and revisit this,
                # perhaps there's something else going on.
    -            "QEMU_MEMORY": 2048,
    +            "QEMU_MEMORY": 4096,
    +            # "QEMU_MEMORY": 2048,
            },
            examples = {}
        ),
    ```
    The issue is under investigation.

3. We don't yet have a proper build system to introduce or paramterise variables for the system, so many of the variables and structures (including the vswitch layout) are hard coded.

# further work
1. add vhost
2. add ethernet and block
3. convert it to a 3-VMM system