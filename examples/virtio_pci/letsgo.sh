#!/bin/sh

BUILD_DIR=/Volumes/scratch/vmm_vio_x86
MICROKIT_SDK=/Users/dreamliner787-9/TS/microkit-capdl-dev/release/microkit-sdk-2.1.0-dev

rm -rfd $BUILD_DIR && \
make MICROKIT_BOARD=x86_64_generic_vtx BUILD_DIR=$BUILD_DIR MICROKIT_SDK=$MICROKIT_SDK -j8 && \
scp /Volumes/scratch/vmm_vio_x86/sel4_32.elf billn@dwarrowdelf.keg.cse.unsw.edu.au:/opt/billn/scratch/sel4_32.elf && \
scp /Volumes/scratch/vmm_vio_x86/loader.img billn@dwarrowdelf.keg.cse.unsw.edu.au:/opt/billn/scratch/loader.img && \
ssh billn@dwarrowdelf.keg.cse.unsw.edu.au "qemu-system-x86_64 -accel kvm -cpu host,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave,+vmx,+vme -kernel /opt/billn/scratch/sel4_32.elf -initrd /opt/billn/scratch/loader.img \
                        -serial mon:stdio \
                        -m size=2G \
                        -nographic \
                        -d guest_errors"

# rm -rfd $BUILD_DIR && \
# make MICROKIT_BOARD=qemu_virt_aarch64 BUILD_DIR=$BUILD_DIR MICROKIT_SDK=$MICROKIT_SDK qemu -j8
