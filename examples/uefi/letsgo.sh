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

# qemu-system-x86_64 \
#   -accel kvm \
#   -cpu Nehalem,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave,+vmx,+vme \
#   -m 512 \
#   -boot d \
#   -drive if=ide,media=cdrom,file=os-limine.iso \
#   -serial mon:stdio --nographic -d guest_errors

# scp board/x86_64_generic_vtx/bzImage billn@dwarrowdelf.keg.cse.unsw.edu.au:/opt/billn/scratch/bzImage && \
# ssh billn@dwarrowdelf.keg.cse.unsw.edu.au "qemu-system-x86_64 -accel kvm -cpu host,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave,+vmx,+vme -kernel /opt/billn/scratch/bzImage \
# -serial mon:stdio --nographic -d guest_errors  -trace ide_* -append 'nokaslr earlyprintk=serial,0x3f8,115200 debug console=ttyS0,115200 earlycon=serial,0x3f8,115200 loglevel=8'"

scp /Volumes/scratch/vmm_x86_uefi/loader.img billn@dwarrowdelf.keg.cse.unsw.edu.au:/opt/billn/scratch/loader.img && \
ssh billn@dwarrowdelf.keg.cse.unsw.edu.au "qemu-system-x86_64 -accel kvm -cpu host,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave,+vmx,+vme -kernel /opt/billn/scratch/sel4_32.elf -initrd /opt/billn/scratch/loader.img \
                        -serial mon:stdio \
                        -m size=2G \
                        -nographic \
                        -d guest_errors \
                        -cdrom /opt/billn/latest-nixos-minimal-x86_64-linux.iso"
    


# qemu-system-x86_64 -cpu qemu64,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave -kernel /Users/dreamliner787-9/TS/libvmm/examples/simple/board/x86_64_generic_vtx/bzImage -initrd /Users/dreamliner787-9/TS/libvmm/examples/simple/board/x86_64_generic_vtx/rootfs.cpio.gz \
#                         -serial mon:stdio \
#                         -m size=2G \
#                         -nographic \
#                         -d guest_errors \
#                         -cdrom /Users/dreamliner787-9/Downloads/nixos-minimal-25.11.1056.d9bc5c7dceb3-x86_64-linux.iso \
#                         -append 'nokaslr earlyprintk=serial,0x3f8,115200 debug console=ttyS0,115200 earlycon=serial,0x3f8,115200 loglevel=8'

# mq.sh run -s skylake -c dafgsdhtvtv -f /Users/dreamliner787-9/TS/microkit-capdl-dev/release/microkit-sdk-2.0.1-dev/board/x86_64_generic_vtx/debug/elf/sel4.elf -f /Volumes/scratch/vmm_x86/loader.img