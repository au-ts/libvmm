const std = @import("std");
const Build = std.Build;
const LazyPath = std.Build.LazyPath;

const MicrokitBoard = enum {
    qemu_arm_virt,
    odroidc4
};

const ConfigOptions = enum {
    debug,
    release,
    benchmark
};

const src = [_][]const u8{
    "src/guest.c",
    "src/util/util.c",
    "src/util/printf.c",
    "src/virtio/mmio.c",
    "src/virtio/console.c",
    "src/virtio/block.c",
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

const uio_blk_driver_src = [_][]const u8{
    "tools/linux/uio/blk.c",
    "tools/linux/uio/libuio.c",
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // Microkit SDK option
    const microkit_sdk_option_tmp = b.option([]const u8, "sdk", "Path to Microkit SDK");
    if (microkit_sdk_option_tmp == null) {
        std.log.err("Missing -Dsdk=/path/to/sdk argument being passed\n", .{});
        std.posix.exit(1);
    }
    const microkit_sdk_option = microkit_sdk_option_tmp.?;
    std.fs.cwd().access(microkit_sdk_option, .{}) catch |err| {
        switch (err) {
            error.FileNotFound => std.log.err("Path to SDK '{s}' does not exist\n", .{ microkit_sdk_option }),
            else => std.log.err("Could not acccess SDK path '{s}', error is {any}\n", .{ microkit_sdk_option, err })
        }
        std.posix.exit(1);
    };

    // Microkit config option
    const microkit_config_option = b.option(ConfigOptions, "config", "Microkit config to build for") orelse ConfigOptions.debug;

    // Microkit board option
    const microkit_board_option_tmp = b.option(MicrokitBoard, "board", "Microkit board to target");
    if (microkit_board_option_tmp == null) {
        std.log.err("Missing -Dboard=<BOARD> argument being passed\n", .{});
        std.posix.exit(1);
    }
    const microkit_board_option = microkit_board_option_tmp.?;
    const microkit_board_dir = b.fmt("{s}/board/{s}/{s}", .{ microkit_sdk_option, @tagName(microkit_board_option), @tagName(microkit_config_option) });
    std.fs.cwd().access(microkit_board_dir, .{}) catch |err| {
        switch (err) {
            error.FileNotFound => std.log.err("Path to '{s}' does not exist\n", .{ microkit_board_dir }),
            else => std.log.err("Could not acccess path '{s}', error is {any}\n", .{ microkit_board_dir, err })
        }
        std.posix.exit(1);
    };

    // Libmicrokit
    const libmicrokit_include = b.fmt("{s}/include", .{ microkit_board_dir });

    // libvmm static ibrary
    const libvmm = b.addStaticLibrary(.{
        .name = "vmm",
        .target = target,
        .optimize = optimize,
    });

    const src_arch = switch (target.result.cpu.arch) {
        .aarch64 => src_aarch64,
        else => {
            std.log.err("Unsupported libvmm architecture given '{s}'", .{ @tagName(target.result.cpu.arch) });
            std.posix.exit(1);
        }
    };
    libvmm.addCSourceFiles(.{
        .files = &(src ++ src_arch),
        .flags = &.{
            "-Wall",
            "-Werror",
            "-Wno-unused-function",
            "-mstrict-align",
            "-fno-sanitize=undefined", // @ivanv: ideally we wouldn't have to turn off UBSAN
            b.fmt("-DBOARD_{s}", .{ @tagName(microkit_board_option) }) // @ivanv: shouldn't be needed as the library should not depend on the board
        }
    });
    // @ivanv: fix all of our libvmm includes! This is a mess!
    libvmm.addIncludePath(b.path("src"));
    libvmm.addIncludePath(b.path("src/util"));
    libvmm.addIncludePath(b.path("src/virtio"));
    libvmm.addIncludePath(b.path("src/arch/aarch64"));
    libvmm.addIncludePath(.{ .cwd_relative = libmicrokit_include });
    const sddf_dep = b.dependency("sddf", .{
        .target = target,
        .optimize = optimize,
        .sdk = microkit_sdk_option,
        .config = @as([]const u8, @tagName(microkit_config_option)),
        .board = @as([]const u8, @tagName(microkit_board_option)),
    });
    libvmm.addIncludePath(sddf_dep.path("include"));
    libvmm.installHeadersDirectory(b.path("src"), "", .{});
    libvmm.installHeadersDirectory(b.path("src/util"), "", .{});
    libvmm.installHeadersDirectory(b.path("src/virtio"), "", .{});
    libvmm.installHeadersDirectory(b.path("src/arch/aarch64"), "", .{});
    b.installArtifact(libvmm);

    // UIO target
    const uio_target_query = switch (target.result.cpu.arch) {
        .aarch64 => .{ .cpu_arch = .aarch64, .os_tag = .linux, .abi = .musl, .cpu_model = .{ .explicit = target.result.cpu.model } },
        else => {
            std.log.err("Unsupported UIO driver architecture given '{s}'", .{ @tagName(target.result.cpu.arch) });
            std.posix.exit(1);
        }
    };
    const uio_target = b.resolveTargetQuery(uio_target_query);

    // UIO block driver
    const uio_blk_driver = b.addExecutable(.{
        .name = "uio_blk_driver",
        .target = uio_target,
        .optimize = optimize,
    });
    uio_blk_driver.addIncludePath(b.path("include"));
    uio_blk_driver.addIncludePath(b.path("tools/linux/include"));
    uio_blk_driver.addIncludePath(sddf_dep.path("include"));
    uio_blk_driver.linkLibC();
    uio_blk_driver.addCSourceFiles(.{
        .files = &(uio_blk_driver_src),
        .flags = &.{
            "-Wall",
            "-Werror",
            "-Wno-unused-function",
            "-mstrict-align",
            "-D_GNU_SOURCE", // Needed for opening files in O_DIRECT to do userspace DMA to block device
        }
    });
    b.installArtifact(uio_blk_driver);
}
