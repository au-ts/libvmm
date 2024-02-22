const std = @import("std");

const src = [_][]const u8{
    "src/guest.c",
    "src/util/util.c",
    "src/util/printf.c",
};

const src_aarch64 = [_][]const u8{
    "src/arch/aarch64/vgic/vgic.c",
    "src/arch/aarch64/vgic/vgic_v2.c",
    "src/arch/aarch64/fault.c",
    "src/arch/aarch64/psci.c",
    "src/arch/aarch64/smc.c",
    "src/arch/aarch64/virq.c",
    "src/arch/aarch64/linux.c",
    "src/arch/aarch64/tcb.c",
    "src/arch/aarch64/vcpu.c",
};

pub fn build(b: *std.Build) void {
    const optimize = b.standardOptimizeOption(.{});
    const target = b.standardTargetOptions(.{});

    // Getting the path to the Microkit SDK before doing anything else
    const microkit_sdk_arg = b.option([]const u8, "sdk", "Path to Microkit SDK");
    if (microkit_sdk_arg == null) {
        std.log.err("Missing -Dsdk=/path/to/sdk argument being passed\n", .{});
        std.os.exit(1);
    }
    const microkit_sdk = microkit_sdk_arg.?;
    std.fs.cwd().access(microkit_sdk, .{}) catch |err| {
        switch (err) {
            error.FileNotFound => std.log.err("Path to SDK '{s}' does not exist\n", .{ microkit_sdk }),
            else => std.log.err("Could not acccess SDK path '{s}', error is {any}\n", .{ microkit_sdk, err })
        }
        std.os.exit(1);
    };

    const microkit_config = b.option([]const u8, "config", "Microkit config to build for") orelse "debug";
    const microkit_board_arg = b.option([]const u8, "board", "Microkit board to target");

    if (microkit_board_arg == null) {
        std.log.err("Missing -Dboard=<BOARD> argument being passed\n", .{});
        std.os.exit(1);
    }
    const microkit_board = microkit_board_arg.?;

    const microkit_board_dir = b.fmt("{s}/board/{s}/{s}", .{ microkit_sdk, microkit_board, microkit_config });
    std.fs.cwd().access(microkit_board_dir, .{}) catch |err| {
        switch (err) {
            error.FileNotFound => std.log.err("Path to '{s}' does not exist\n", .{ microkit_board_dir }),
            else => std.log.err("Could not acccess path '{s}', error is {any}\n", .{ microkit_board_dir, err })
        }
        std.os.exit(1);
    };
    const libmicrokit_include = b.fmt("{s}/include", .{ microkit_board_dir });

    const libvmm = b.addStaticLibrary(.{
        .name = "vmm",
        .target = target,
        .optimize = optimize,
    });

    const src_arch = switch (target.result.cpu.arch) {
        .aarch64 => src_aarch64,
        else => {
            std.log.err("Unsupported libvmm architecture given '{s}'", .{ @tagName(target.result.cpu.arch) });
            std.os.exit(1);
        }
    };
    libvmm.addCSourceFiles(.{
        .files = &(src ++ src_arch),
        .flags = &.{
            "-Wall",
            "-Werror",
            "-Wno-unused-function",
            "-mstrict-align",
            "-fno-sanitize=undefined",
            b.fmt("-DBOARD_{s}", .{ microkit_board }) // @ivanv: shouldn't be needed as the library should not depend on the board
        }
    });

    // @ivanv: fix all of our libvmm includes! This is a mess!
    libvmm.addIncludePath(.{ .path = "src" });
    libvmm.addIncludePath(.{ .path = "src/util/" });
    libvmm.addIncludePath(.{ .path = "src/arch/aarch64" });
    libvmm.addIncludePath(.{ .path = "src/arch/aarch64/vgic/" });
    libvmm.addIncludePath(.{ .path = libmicrokit_include });

    b.installArtifact(libvmm);
}
