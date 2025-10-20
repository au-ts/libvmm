#!/bin/sh

rm -rfd /Volumes/scratch/vmm_x86 && \

make MICROKIT_BOARD=x86_64_generic_vtx BUILD_DIR='/Volumes/scratch/vmm_x86' MICROKIT_SDK=/Users/dreamliner787-9/TS/microkit-capdl-dev/release/microkit-sdk-2.0.1-dev && \

# scp /Volumes/scratch/vmm_x86/loader.img billn@dwarrowdelf.keg.cse.unsw.edu.au:/opt/billn/scratch/loader.img && \
# scp /Volumes/scratch/vmm_x86/sel4_32b.elf billn@dwarrowdelf.keg.cse.unsw.edu.au:/opt/billn/scratch/sel4_32b.elf && \
# ssh billn@dwarrowdelf.keg.cse.unsw.edu.au "qemu-system-x86_64 -accel kvm -cpu Nehalem,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave,+vmx,+vme -kernel /opt/billn/scratch/sel4_32b.elf -initrd /opt/billn/scratch/loader.img \
#                         -serial mon:stdio \
#                         -m size=2G \
#                         -nographic -d guest_errors"

mkdir -p /Volumes/scratch/vmm_x86/iso/boot/ && \
mkdir -p /Volumes/scratch/vmm_x86/iso/EFI/BOOT && \
cp /Users/dreamliner787-9/TS/microkit-capdl-dev/release/microkit-sdk-2.0.1-dev/board/x86_64_generic_vtx/debug/elf/sel4.elf /Volumes/scratch/vmm_x86/iso/boot/kernel.elf && \
cp /Volumes/scratch/vmm_x86/loader.img /Volumes/scratch/vmm_x86/iso/boot/loader.elf && \
cp /Users/dreamliner787-9/TS/libvmm/examples/simple/limine.cfg  /Volumes/scratch/vmm_x86/iso/limine.conf && \
cp /Users/dreamliner787-9/TS/libvmm/examples/simple/Limine/limine-bios-cd.bin /Volumes/scratch/vmm_x86/iso/limine-bios-cd.bin && \
cp /Users/dreamliner787-9/TS/libvmm/examples/simple/Limine/limine-bios.sys /Volumes/scratch/vmm_x86/iso/limine-bios.sys && \
cp /Users/dreamliner787-9/TS/libvmm/examples/simple/Limine/limine-uefi-cd.bin /Volumes/scratch/vmm_x86/iso/limine-uefi-cd.bin && \
cp /Users/dreamliner787-9/TS/libvmm/examples/simple/Limine/BOOTX64.EFI /Volumes/scratch/vmm_x86/iso/EFI/BOOT/BOOTX64.EFI && \
xorriso -as mkisofs -R -r -J -b limine-bios-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table -hfsplus \
        -apm-block-size 2048 --efi-boot limine-uefi-cd.bin \
        -efi-boot-part --efi-boot-image --protective-msdos-label \
        /Volumes/scratch/vmm_x86/iso/ -o /Volumes/scratch/vmm_x86/os-limine.iso && \
./Limine/limine bios-install /Volumes/scratch/vmm_x86/iso/limine-bios-cd.bin && \
cp /Users/dreamliner787-9/TS/libvmm/examples/simple/boshsrc /Volumes/scratch/vmm_x86/boshsrc && \
cd /Volumes/scratch/vmm_x86/ && \
bochs -q -f /Volumes/scratch/vmm_x86/boshsrc
cd -
# qemu-system-x86_64 \
#   -cpu qemu64,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave \
#   -m 512 \
#   -vga std \
#   -boot d \
#   -drive if=ide,media=cdrom,file=/Volumes/scratch/vmm_x86/os-limine.iso \
#   -serial stdio



# mq.sh run -s skylake -c dafgsdhtvtv -f /Users/dreamliner787-9/TS/microkit-capdl-dev/release/microkit-sdk-2.0.1-dev/board/x86_64_generic_vtx/debug/elf/sel4.elf -f /Volumes/scratch/vmm_x86/loader.img