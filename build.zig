const std = @import("std");

const src = [_][]const u8{
    "src/guest.c",
    "src/util/util.c",
    "src/util/printf.c",
};

const src_aarch64_vgic_v2 = [_][]const u8{
    "src/arch/aarch64/vgic/vgic_v2.c",
};

const src_aarch64_vgic_v3 = [_][]const u8{
    "src/arch/aarch64/vgic/vgic_v3.c",
};

const src_aarch64 = [_][]const u8{
    "src/arch/aarch64/vgic/vgic.c",
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

    const libmicrokit_include_opt = b.option([]const u8, "libmicrokit_include", "Path to the libmicrokit include directory") orelse null;
    const microkit_board_opt = b.option([]const u8, "microkit_board", "Name of Microkit board") orelse null;

    // Default to vGIC version 2
    const arm_vgic_version = b.option(usize, "arm_vgic_version", "ARM vGIC version to emulate") orelse null;

    const libmicrokit_include = libmicrokit_include_opt.?;

    const libvmm = b.addStaticLibrary(.{
        .name = "vmm",
        .target = target,
        .optimize = optimize,
    });

    const src_arch = switch (target.result.cpu.arch) {
        .aarch64 => blk: {
            const vgic_src = switch (arm_vgic_version.?) {
                2 => src_aarch64_vgic_v2,
                3 => src_aarch64_vgic_v3,
                else => @panic("Unsupported vGIC version given"),
            };

            break :blk src_aarch64 ++ vgic_src;
        },
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
            b.fmt("-DBOARD_{s}", .{ microkit_board_opt.? }) // @ivanv: shouldn't be needed as the library should not depend on the board
        }
    });

    // @ivanv: fix all of our libvmm includes! This is a mess!
    libvmm.addIncludePath(b.path("src"));
    libvmm.addIncludePath(b.path("src/util/"));
    libvmm.addIncludePath(b.path("src/arch/aarch64"));
    libvmm.addIncludePath(b.path("src/arch/aarch64/vgic/"));
    libvmm.addIncludePath(.{ .cwd_relative = libmicrokit_include });

    b.installArtifact(libvmm);
}
