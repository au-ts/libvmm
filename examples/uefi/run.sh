qemu-system-x86_64 -accel kvm -cpu host,+sse,+sse2,+fsgsbase,+pdpe1gb,+xsaveopt,+xsave,+vmx,+vme \
					  -device virtio-net-pci,netdev=netdev0,addr=0x2.0 \
					  -device virtio-blk-pci,drive=drive0,id=virtblk0,num-queues=1,addr=0x3.0 \
                      -serial mon:stdio \
							  -m size=3G \
							  -device ramfb -vga none \
							  -drive file=/dev/loop12,format=raw,if=virtio,id=drive0 \
							  -netdev user,id=netdev0,hostfwd=tcp::1236-:1236,hostfwd=tcp::1237-:1237,hostfwd=udp::1235-:1235 \
                              -bios /home/billn/ts/libvmm/examples/uefi/board/x86_64_generic_vtx/OVMF.fd