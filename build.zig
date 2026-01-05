// Copyright 2024, UNSW
// SPDX-License-Identifier: BSD-2-Clause

const std = @import("std");
const LazyPath = std.Build.LazyPath;

const src = [_][]const u8{
    "src/guest.c",
    "src/util/util.c",
    "src/util/printf.c",
    "src/virtio/mmio.c",
    "src/virtio/pci.c",
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
    "src/arch/aarch64/vgic/vgic_v3_cpuif.c",
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
    "src/arch/aarch64/cpuif.c",
};

const src_x86_64 = [_][]const u8{
    "src/arch/x86_64/linux.c",
    "src/arch/x86_64/fault.c",
    "src/arch/x86_64/vcpu.c",
    "src/arch/x86_64/virq.c",
    "src/arch/x86_64/cpuid.c",
    "src/arch/x86_64/msr.c",
    "src/arch/x86_64/acpi.c",
    "src/arch/x86_64/apic.c",
    "src/arch/x86_64/hpet.c",
    "src/arch/x86_64/instruction.c",
    "src/arch/x86_64/pci.c",
    "src/arch/x86_64/pit.c",
    "src/arch/x86_64/ioports.c",
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

        var srcs = std.ArrayList([]const u8){};
        defer srcs.deinit(b.allocator);

        try srcs.appendSlice(b.allocator, &src);

        switch (target.result.cpu.arch) {
            .aarch64 => {
                switch (arm_vgic_version.?) {
                    2 => try srcs.appendSlice(b.allocator, &src_aarch64_vgic_v2),
                    3 => try srcs.appendSlice(b.allocator, &src_aarch64_vgic_v3),
                    else => @panic("Unsupported vGIC version given"),
                }

                try srcs.appendSlice(b.allocator, &src_aarch64);
            },
            .x86_64 => {
                try srcs.appendSlice(b.allocator, &src_x86_64);
            },
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
                // "-mstrict-align",
                // "-fno-sanitize=undefined", // https://github.com/au-ts/libvmm/issues/35
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
                // "-mstrict-align",
                // "-fno-sanitize=undefined", // https://github.com/au-ts/libvmm/issues/35
            }
        });
        libuio.addIncludePath(sddf.path("include"));
        libuio.linkLibC();
        b.installArtifact(libuio);
    }
}
