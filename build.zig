// Copyright 2024, UNSW
// SPDX-License-Identifier: BSD-2-Clause

const std = @import("std");
const LazyPath = std.Build.LazyPath;

const src = [_][]const u8{
    "src/guest.c",
    "src/fault.c",
    "src/util/util.c",
    "src/util/printf.c",
    "src/virtio/mmio.c",
    "src/virtio/block.c",
    "src/virtio/console.c",
    "src/virtio/net.c",
    "src/virtio/sound.c",
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

const src_riscv = [_][]const u8{
    "src/arch/riscv/fault.c",
    "src/arch/riscv/sbi.c",
    "src/arch/riscv/linux.c",
    "src/arch/riscv/tcb.c",
    "src/arch/riscv/vcpu.c",
    "src/arch/riscv/virq.c",
    "src/arch/riscv/plic.c",
};

/// Convert the target for Microkit (e.g freestanding AArch64 or RISC-V) to the Linux
/// equivalent. Assumes musllibc will be used.
fn linuxTarget(b: *std.Build, target: std.Build.ResolvedTarget) std.Build.ResolvedTarget {
    var query = target.query;
    query.os_tag = .linux;
    query.abi = .musl;

    return b.resolveTargetQuery(query);
}

pub fn build(b: *std.Build) !void {
    const optimize = b.standardOptimizeOption(.{});
    const target = b.standardTargetOptions(.{});

    // Default to vGIC version 2
    const arm_vgic_version = b.option(usize, "arm_vgic_version", "ARM vGIC version to emulate") orelse null;

    const maybe_microkit_board_dir = b.option(LazyPath, "microkit_board_dir", "Path within Microkit SDK for the target board") orelse null;
    if (maybe_microkit_board_dir) |microkit_board_dir| {
        const libmicrokit_include = microkit_board_dir.path(b, "include");

        const libvmm = b.addLibrary(.{
            .name = "vmm",
            .root_module = b.createModule(.{
                .target = target,
                .optimize = optimize,
            }),
        });

        const sddf = b.dependency("sddf", .{
            .target = target,
            .optimize = optimize,
            .microkit_board_dir = microkit_board_dir
        });

        var srcs = std.ArrayList([]const u8).init(b.allocator);
        defer srcs.deinit();

        try srcs.appendSlice(&src);

        switch (target.result.cpu.arch) {
            .aarch64 => {
                const vgic_src = switch (arm_vgic_version.?) {
                    2 => src_aarch64_vgic_v2,
                    3 => src_aarch64_vgic_v3,
                    else => @panic("Unsupported vGIC version given"),
                };
                try srcs.appendSlice(&src_aarch64);
                try srcs.appendSlice(&vgic_src);
            },
            .riscv64 => try srcs.appendSlice(&src_riscv),
            else => {
                std.log.err("Unsupported libvmm architecture given '{s}'", .{ @tagName(target.result.cpu.arch) });
                std.posix.exit(1);
            }
        }
        libvmm.addCSourceFiles(.{
            .files = srcs.items,
            .flags = &.{
                "-Wall",
                "-Werror",
                "-Wno-unused-function",
                "-mstrict-align",
                "-fno-sanitize=undefined", // https://github.com/au-ts/libvmm/issues/35
            }
        });

        libvmm.addIncludePath(b.path("include"));
        libvmm.addIncludePath(sddf.path("include"));
        libvmm.addIncludePath(sddf.path("include/microkit"));
        libvmm.addIncludePath(libmicrokit_include);

        libvmm.installHeadersDirectory(b.path("include/libvmm"), "libvmm", .{});
        libvmm.installHeadersDirectory(sddf.path("include/microkit/os"), "os", .{});
        libvmm.installHeadersDirectory(sddf.path("include/sddf"), "sddf", .{});

        libvmm.linkLibrary(sddf.artifact("util"));
        libvmm.linkLibrary(sddf.artifact("util_putchar_debug"));

        b.installArtifact(libvmm);

        const libuio = b.addLibrary(.{
            .name = "uio",
            .root_module = b.createModule(.{
                .target = linuxTarget(b, target),
                .optimize = optimize,
            }),
        });
        libuio.addCSourceFile(.{
            .file = b.path("tools/linux/uio/libuio.c"),
            .flags = &.{
                "-Wall",
                "-Werror",
                "-Wno-unused-function",
                "-mstrict-align",
                "-fno-sanitize=undefined", // https://github.com/au-ts/libvmm/issues/35
            }
        });
        libuio.addIncludePath(sddf.path("include"));
        libuio.linkLibC();
        b.installArtifact(libuio);
    }
}
