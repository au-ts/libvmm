#!/bin/sh

# ./configure --with-nogui --enable-vmx=2 --enable-avx=yes --enable-x86-64=yes --enable-debugger && make -j10 && sudo make install

BUILD_DIR=/Volumes/scratch/vmm_x86_uefi
MICROKIT_SDK=/Users/dreamliner787-9/TS/microkit-capdl-dev/release/microkit-sdk-2.1.0-dev

EXAMPLE_DIR=$(pwd)
SYSTEM_DIR=$EXAMPLE_DIR/board/x86_64_generic_vtx/
BOOTLOADER_CFG=$SYSTEM_DIR/limine.cfg
BOOTLOADER=$SYSTEM_DIR/Limine
BOSHRC=$SYSTEM_DIR/boshsrc
KERNEL64_ELF=$BUILD_DIR/sel4.elf
ISO_STAGING_DIR=$BUILD_DIR/iso

rm -rfd $BUILD_DIR && \

scp billn@dwarrowdelf.keg.cse.unsw.edu.au:/opt/billn/edk2/Build/OvmfX64/DEBUG_GCC/FV/OVMF.fd /Users/dreamliner787-9/TS/libvmm/examples/uefi/board/x86_64_generic_vtx/OVMF.fd && \
make MICROKIT_BOARD=x86_64_generic_vtx BUILD_DIR=$BUILD_DIR MICROKIT_SDK=$MICROKIT_SDK && \

# mkdir -p $ISO_STAGING_DIR/boot/ && \
# mkdir -p $ISO_STAGING_DIR/EFI/BOOT && \
# cp $KERNEL64_ELF $ISO_STAGING_DIR/boot/kernel.elf && \
# cp $BUILD_DIR/loader.img $ISO_STAGING_DIR/boot/loader.elf && \
# cp $BOOTLOADER_CFG $ISO_STAGING_DIR/limine.conf && \
# cp $BOOTLOADER/limine-bios-cd.bin $ISO_STAGING_DIR/limine-bios-cd.bin && \
# cp $BOOTLOADER/limine-bios.sys $ISO_STAGING_DIR/limine-bios.sys && \
# cp $BOOTLOADER/limine-uefi-cd.bin $ISO_STAGING_DIR/limine-uefi-cd.bin && \
# cp $BOOTLOADER/BOOTX64.EFI $ISO_STAGING_DIR/EFI/BOOT/BOOTX64.EFI && \
# xorriso -as mkisofs -R -r -J -b limine-bios-cd.bin \
#         -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
#         -apm-block-size 2048 --efi-boot limine-uefi-cd.bin \
#         -efi-boot-part --efi-boot-image --protective-msdos-label \
#         $ISO_STAGING_DIR -o $BUILD_DIR/os-limine.iso && \
# cd $BUILD_DIR && \
# echo c | bochs -q -f $BOSHRC
# cd -



# how to reproduce OVMF.fd, only works on linux, doesnt compile on mac :(
# sudo apt install build-essential uuid-dev iasl git  nasm  python-is-python3
# $ git clone https://github.com/tianocore/edk2
# $ cd edk2/
# $ git submodule update --init

# add to edk2/OvmfPkg/OvmfPkgX64.dsc if you want debug print to com1 serial port
# [PcdsFixedAtBuild]
# gEfiMdeModulePkgTokenSpaceGuid.PcdSerialRegisterBase | 0x3F8
# gEfiMdeModulePkgTokenSpaceGuid.PcdSerialBaudRate     | 115200
# gEfiMdeModulePkgTokenSpaceGuid.PcdSerialUseMmio      | FALSE

# $ make -C BaseTools
# $ ./OvmfPkg/build.sh -a X64 -b DEBUG -p OvmfPkg/OvmfPkgX64.dsc -D DEBUG_ON_SERIAL_PORT
# $ ls -hl /opt/billn/edk2/Build/OvmfX64/DEBUG_GCC/FV/


# qemu-system-x86_64 \
#   -accel kvm -cpu host,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave,+vmx,+vme \
#   -m size=2G \
#   -drive if=pflash,format=raw,file=Build/OvmfX64/DEBUG_GCC/FV/OVMF.fd \
#   -serial mon:stdio \
#   -nographic



scp /Volumes/scratch/vmm_x86_uefi/sel4_32.elf billn@dwarrowdelf.keg.cse.unsw.edu.au:/opt/billn/scratch/sel4_32.elf && \
scp /Volumes/scratch/vmm_x86_uefi/loader.img billn@dwarrowdelf.keg.cse.unsw.edu.au:/opt/billn/scratch/loader.img && \
ssh -X billn@dwarrowdelf.keg.cse.unsw.edu.au "qemu-system-x86_64 -accel kvm -cpu host,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave,+vmx,+vme -kernel /opt/billn/scratch/sel4_32.elf -initrd /opt/billn/scratch/loader.img \
                        -serial mon:stdio \
                        -m size=2G \
                        -d guest_errors \
                        -cdrom /opt/billn/latest-nixos-minimal-x86_64-linux.iso" \
                        -device ramfb
    


