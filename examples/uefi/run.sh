qemu-system-x86_64 -accel kvm -cpu host,+sse,+sse2,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave,+vmx,+vme \
					  -device virtio-net-pci,netdev=netdev0,addr=0x2.0 \
                      -serial mon:stdio \
							  -m size=3G \
							  -device ramfb -vga none \
							  -cdrom /home/billn/Downloads/EaseUS.Partition.Master.14.5.WinPE.iso \
							  -netdev user,id=netdev0,hostfwd=tcp::1236-:1236,hostfwd=tcp::1237-:1237,hostfwd=udp::1235-:1235 \
                              -bios /home/billn/ts/libvmm/examples/uefi/board/x86_64_generic_vtx/OVMF.fd

# 					  -device virtio-blk-pci,drive=drive0,id=virtblk0,num-queues=1,addr=0x3.0 \

							  # -drive file=/dev/loop12,format=raw,if=virtio,id=drive0 \



qemu-system-x86_64 -machine q35,hpet-msi=off -cpu qemu64,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave \
 -kernel /home/billn/ts/libvmm/examples/simple/board/x86_64_generic_vtx/bzImage\
 -initrd /home/billn/ts/libvmm/examples/simple/board/x86_64_generic_vtx/rootfs.cpio.gz \
 -serial mon:stdio --nographic -d guest_errors \
  -append 'nokaslr earlyprintk=serial,0x3f8,115200 debug console=ttyS0,115200 earlycon=serial,0x3f8,115200 loglevel=8 pci=nomsi clocksource=hpet'
