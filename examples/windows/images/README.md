<!--
     Copyright 2026, UNSW
     SPDX-License-Identifier: CC-BY-SA-4.0
-->

# Guest images used

## TianoCore EDK II Open Virtual Machine Firmware (OVMF)

### Details

* Git remote: https://github.com/tianocore/edk2.git
* Tag: edk2-stable202605 (commit hash: `b03a21a63e3bd001f52c527e5a57feddb53a690b`)

### Building

1. Install the build dependencies:
```
sudo apt install build-essential uuid-dev iasl git nasm python-is-python3
```

2. Get the sources:
```
git clone --depth 1 --branch edk2-stable202605 https://github.com/tianocore/edk2
cd edk2
git submodule update --init
```

3. Enable debug logging. By default the EDK II UEFI firmware will output
debug logs to a special I/O Port that QEMU handles, but we do not implement
that special port, so we need to tell the firmware to just use standard COM1.
Append this block to the end of `OvmfPkg/OvmfPkgX64.dsc`:
```
[PcdsFixedAtBuild]
gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase | 0x3F8
gEfiMdeModulePkgTokenSpaceGuid.PcdSerialBaudRate     | 115200
gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio      | FALSE
```

4. If you intend to deploy the VM with video capability, you can replace the
TianoCore logo on the boot splash screen with one of your own. Our prebuilt
image ships with the LionsOS logo. To use a custom logo, overwrite
MdeModulePkg/Logo/Logo.bmp with an uncompressed 8-bit RGB bitmap with no alpha channel.

5. Build:
```
make -C BaseTools
./OvmfPkg/build.sh -a X64 -b DEBUG -p OvmfPkg/OvmfPkgX64.dsc -D DEBUG_ON_SERIAL_PORT
```

The binary will be located at:
```
ls -hl Build/OvmfX64/DEBUG_GCC/FV/OVMF.fd
```

## Unattend Answer File

Used to automate the Windows 11 installation process. An answer file is an
XML file placed on a disk that the machine can mount. When the Windows
installer boots, it looks for this file and applies the
installation settings it contains.

Generated with <https://schneegans.de/windows/unattend-generator/> (commit 662d88d).

To reset the web form to the values we used, import autounattend.xml.
