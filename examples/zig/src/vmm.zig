// Copyright 2023, UNSW
// SPDX-License-Identifier: BSD-2-Clause

const std = @import("std");
const c = @cImport({
    @cInclude("linux.h");
    @cInclude("virq.h");
    @cInclude("guest.h");
    @cInclude("vcpu.h");
    @cInclude("tcb.h");
});
const sel4cp = @import("libsel4cp.zig");

const GUEST_BOOT_VCPU_ID = 0;
const GUEST_RAM_VADDR: usize = 0x40000000;
const GUEST_DTB_VADDR: usize = 0x4f000000;
const GUEST_INIT_RAM_DISK_VADDR: usize = 0x4d700000;
const GUEST_RAM_SIZE: usize = 0x10000000;

const guest_image_path = "images/";
// Data for the guest's kernel image.
const guest_kernel_image = blk: {
    const arr align(@alignOf(c.linux_image_header)) = @embedFile(guest_image_path ++ "linux").*;
    break :blk &arr;
};
// Data for the device tree to be passed to the kernel.
const guest_dtb_image = @embedFile(guest_image_path ++ "linux.dtb");
// Data for the initial RAM disk to be passed to the kernel.
const guest_initrd_image = @embedFile(guest_image_path ++ "rootfs.cpio.gz");

const LinuxKernelImage = extern struct {
    header: c.linux_image_header,
    bytes: *u8
};

// In Zig the standard library comes with printf-like functionality with the
// ability to provide your own function to ouput the characters. This is
// extremely useful for us! Without changing the standard library or bringing
// in a 3rd party library, in a couple of lines of code we can get formatted
// printing functionality that ends up calling to sel4cp_dbg_puts which then
// outputs to the platform's serial connection.
//
// In a production system you might want to instead hook up the logger to a
// user-level UART driver, but for the example we only need logging for debug
// mode.
const log = struct {
    const Writer = std.io.Writer(u32, error{}, debug_uart_put_str);
    const debug_uart = Writer { .context = 0 };

    fn debug_uart_put_str(_: u32, str: []const u8) !usize {
        sel4cp.sel4cp_dbg_puts(@ptrCast(str));
        return str.len;
    }

    pub fn info(comptime fmt: []const u8, args: anytype) void {
        debug_uart.print("VMM|INFO: " ++ fmt ++ "\n", args) catch {};
    }

    pub fn err(comptime fmt: []const u8, args: anytype) void {
        debug_uart.print("VMM|ERROR: " ++ fmt ++ "\n", args) catch {};
    }
};

const SERIAL_IRQ_CH: sel4cp.sel4cp_channel = 1;
const SERIAL_IRQ: i32 = 79;

fn serial_ack(_: usize, irq: c_int, _: ?*anyopaque) callconv(.C) void {
    // Nothing else needs to be done other than acking the IRQ.
    sel4cp.sel4cp_irq_ack(@intCast(irq));
}

export fn init() callconv(.C) void {
    // Initialise the VMM, the VCPU(s), and start the guest
    log.info("starting \"{s}\"\n", .{ sel4cp.sel4cp_name });
    // Place all the binaries in the right locations before starting the guest
    // @ivanv: should all the vmm functions be prefixed with "vmm_"?
    // var linux_kernel_image: *LinuxKernelImage = @alignCast(@constCast(@ptrCast(guest_kernel_image)));
    // log.info("addr of linux_kernel_image: {*}", .{ linux_kernel_image });
    const kernel_pc = c.linux_setup_images(
                GUEST_RAM_VADDR,
                @intFromPtr(guest_kernel_image),
                guest_kernel_image.len,
                @intFromPtr(guest_dtb_image),
                GUEST_DTB_VADDR,
                guest_dtb_image.len,
                @intFromPtr(guest_initrd_image),
                GUEST_INIT_RAM_DISK_VADDR,
                guest_initrd_image.len
            );

    if (kernel_pc == 0) {
        log.err("Failed to initialise guest images\n", .{});
        return;
    }
    // Initialise the virtual GIC driver
    var success = c.virq_controller_init(GUEST_BOOT_VCPU_ID);
    if (!success) {
        log.err("Failed to initialise emulated interrupt controller\n", .{});
        return;
    }
    // @ivanv: Note that remove this line causes the VMM to fault if we
    // actually get the interrupt. This should be avoided by making the VGIC driver more stable.
    success = c.virq_register(GUEST_BOOT_VCPU_ID, SERIAL_IRQ, &serial_ack, null);
    // Just in case there is already an interrupt available to handle, we ack it here.
    sel4cp.sel4cp_irq_ack(SERIAL_IRQ_CH);
    // Finally start the guest
    success = c.guest_start(GUEST_BOOT_VCPU_ID, kernel_pc, GUEST_DTB_VADDR, GUEST_INIT_RAM_DISK_VADDR);
    std.debug.assert(success);
}

export fn notified(ch: sel4cp.sel4cp_channel) callconv(.C) void {
    switch (ch) {
        SERIAL_IRQ_CH => {
            const success = c.virq_inject(GUEST_BOOT_VCPU_ID, SERIAL_IRQ);
            if (!success) {
                log.err("IRQ {x} dropped on vCPU {x}\n", .{ SERIAL_IRQ, GUEST_BOOT_VCPU_ID });
            }
        },
        else => log.err("Unexpected channel, ch: {x}\n", .{ ch })
    }
}

// @ivanv deal with
extern fn fault_handle_vm_exception(vcpu_id: usize) callconv(.C) bool;
extern fn fault_handle_unknown_syscall(vcpu_id: usize) callconv(.C) bool;
extern fn fault_handle_user_exception(vcpu_id: usize) callconv(.C) bool;
extern fn fault_handle_vgic_maintenance(vcpu_id: usize) callconv(.C) bool;
extern fn fault_handle_vcpu_exception(vcpu_id: usize) callconv(.C) bool;
extern fn fault_handle_vppi_event(vcpu_id: usize) callconv(.C) bool;
extern fn fault_to_string(fault_label: usize) callconv(.C) [*c]u8;

export fn fault(id: sel4cp.sel4cp_id, msginfo: sel4cp.sel4cp_msginfo) callconv(.C) void {
    if (id != GUEST_BOOT_VCPU_ID) {
        log.err("Unexpected faulting PD/VM with ID {}\n", .{ id });
        return;
    }
    // This is the primary fault handler for the guest, all faults that come
    // from seL4 regarding the guest will need to be handled here.
    const label = sel4cp.sel4cp_msginfo_get_label(msginfo);
    log.info("fault label: {}", .{ label });
    const success = switch (label) {
        sel4cp.seL4_Fault_VMFault => fault_handle_vm_exception(id),
        sel4cp.seL4_Fault_UnknownSyscall => fault_handle_unknown_syscall(id),
        sel4cp.seL4_Fault_UserException => fault_handle_user_exception(id),
        sel4cp.seL4_Fault_VGICMaintenance => fault_handle_vgic_maintenance(id),
        sel4cp.seL4_Fault_VCPUFault => fault_handle_vcpu_exception(id),
        sel4cp.seL4_Fault_VPPIEvent => fault_handle_vppi_event(id),
        else => {
            // We have reached a genuinely unexpected case, stop the guest.
            log.err("unknown fault label {x}, stopping guest with ID {x}", .{ label, id });
            sel4cp.sel4cp_vm_stop(id);
            // Dump the TCB and vCPU registers to hopefully get information as
            // to what has gone wrong.
            c.tcb_print_regs(id);
            c.vcpu_print_regs(id);
            return;
        }
    };

    if (success) {
        // Now that we have handled the fault, we reply to it so that the guest can resume execution.
        sel4cp.sel4cp_fault_reply(sel4cp.sel4cp_msginfo_new(0, 0));
    } else {
        log.err("Failed to handle {s} fault\n", .{ fault_to_string(label) });
    }
}
