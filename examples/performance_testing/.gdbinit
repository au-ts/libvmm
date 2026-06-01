# .gdbinit
file /home/alexkv/thesis/microkit-sdk-2.2.0-dev/board/qemu_virt_aarch64/benchmark/elf/sel4.elf
add-symbol-file build/vmm.elf
add-symbol-file build/benchmark.elf 
add-symbol-file build/serial_driver.elf
add-symbol-file build/serial_virt_rx.elf
add-symbol-file build/serial_virt_tx.elf



target remote :1234

# break vmm.c:notified
# break vmm.c:init
break vmm.c:132
break vmm.c:notified
break /home/alexkv/thesis/libvmm/include/libvmm/arch/aarch64/vgic/virq.h:219

# Set hardware breakpoints on the Microkit entry points
# hbreak init
# hbreak main
# hbreak fault

# Set a hardware breakpoint on the function that jumps from kernel to userspace
# hbreak restore_user_context

# Or break on the kernel's fault handler
# hbreak handleVCPUFault
# hbreak handleFault