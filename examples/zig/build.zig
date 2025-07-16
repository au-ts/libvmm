// Copyright 2024, UNSW
// SPDX-License-Identifier: BSD-2-Clause
const std = @import("std");
const LazyPath = std.Build.LazyPath;

const ConfigOptions = enum {
    debug,
    release,
};

pub fn build(b: *std.Build) !void {
    // For this example we hard-code the target to AArch64 and the platform to QEMU virt AArch64
    // since the main point of this example is to show off using libvmm in another
    // systems programming language.
    const target = b.resolveTargetQuery(.{
        .cpu_arch = .aarch64,
        .cpu_model = .{ .explicit = &std.Target.aarch64.cpu.cortex_a53 },
        .cpu_features_add = std.Target.aarch64.featureSet(&[_]std.Target.aarch64.Feature{ .strict_align }),
        .os_tag = .freestanding,
        .abi = .none,
    });
    const optimize = b.standardOptimizeOption(.{});

    // Getting the path to the Microkit SDK before doing anything else
    const microkit_sdk = b.option(LazyPath, "sdk", "Path to Microkit SDK") orelse {
        std.log.err("Missing -Dsdk=/path/to/sdk argument", .{});
        return error.MissingMicrokitSdk;
    };

    // Hard-code the board to QEMU virt AArch64 since it's the only one the example intends to support
    const microkit_board = "qemu_virt_aarch64";
    const microkit_config_option = b.option(ConfigOptions, "config", "Microkit config to build for") orelse ConfigOptions.debug;
    const microkit_config = @tagName(microkit_config_option);
    // Since we are relying on Zig to produce the final ELF, it needs to do the
    // linking step as well.
    const microkit_board_dir = microkit_sdk.path(b, "board").path(b, microkit_board).path(b, microkit_config);
    const microkit_tool = microkit_sdk.path(b, "bin/microkit");
    const libmicrokit = microkit_board_dir.path(b, "lib/libmicrokit.a");
    const libmicrokit_include = microkit_board_dir.path(b, "include");
    const libmicrokit_linker_script = microkit_board_dir.path(b, "lib/microkit.ld");

    const zig_libmicrokit = b.addObject(.{
        .name = "zig_libmicrokit",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });
    zig_libmicrokit.addCSourceFile(.{ .file = b.path("src/extern.c"), .flags = &.{} });
    zig_libmicrokit.addIncludePath(b.path("src/"));
    zig_libmicrokit.addIncludePath(libmicrokit_include);

    const libvmm_dep = b.dependency("libvmm", .{
        .target = target,
        .optimize = optimize,
        .microkit_board_dir = microkit_board_dir,
        // Because we only support QEMU virt AArch64, vGIC version is always 2.
        .arm_vgic_version = @as(usize, 2),
    });
    const libvmm = libvmm_dep.artifact("vmm");

    const exe = b.addExecutable(.{
        .name = "vmm.elf",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/vmm.zig"),
            .target = target,
            .optimize = optimize,
            // Microkit expects and requires the symbol table to exist in the ELF,
            // this means that even when building for release mode, we want to tell
            // Zig not to strip symbols from the binary.
            .strip = false,
        }),
    });

    // For actually compiling the DTS into a DTB
    const dtc_cmd = b.addSystemCommand(&[_][]const u8{
        "dtc", "-q", "-I", "dts", "-O", "dtb"
    });
    dtc_cmd.addFileArg(b.path("images/linux.dts"));
    const dtb = dtc_cmd.captureStdOut();
    // The artefacts that are pre-compiled (Linux kernel and init RAM disk) are
    // dependencies that are fetched by the build process. However the user can specify
    // the build system to use their copy instead.
    const custom_linux = b.option([]const u8, "linux", "Custom Linux image to use") orelse null;
    const custom_initrd = b.option([]const u8, "initrd", "Custom initrd image to use") orelse null;

    const linux_kernel = b.lazyDependency("linux", .{});
    const initrd = b.lazyDependency("initrd", .{});

    if (custom_linux) |c| {
        exe.step.dependOn(&b.addInstallFileWithDir(.{ .cwd_relative = c }, .prefix, "linux").step);
    } else if (linux_kernel) |linux_image| {
        exe.step.dependOn(&b.addInstallFileWithDir(linux_image.path("linux"), .prefix, "linux").step);
    }

    if (custom_initrd) |c| {
        exe.step.dependOn(&b.addInstallFileWithDir(.{ .cwd_relative = c }, .prefix, "rootfs.cpio.gz").step);
    } else if (initrd) |initrd_image| {
        exe.step.dependOn(&b.addInstallFileWithDir(initrd_image.path("rootfs.cpio.gz"), .prefix, "rootfs.cpio.gz").step);
    }

    // When we embed these artifacts into our VMM code, we use @embedFile provided by
    // the Zig compiler. However, we can't just include any path outside of the 'src/'
    // directory and so we add each file as a "module".
    exe.root_module.addAnonymousImport("dtb", .{ .root_source_file = dtb });
    exe.root_module.addAnonymousImport("linux", .{ .root_source_file = .{ .cwd_relative = b.getInstallPath(.prefix, "linux") } });
    exe.root_module.addAnonymousImport("initrd", .{ .root_source_file = .{ .cwd_relative = b.getInstallPath(.prefix, "rootfs.cpio.gz") } });

    exe.addIncludePath(b.path("src/"));
    // Add microkit.h to be used by the API wrapper.
    exe.addIncludePath(libmicrokit_include);
    // Add the static library that provides each protection domain's entry
    // point (`main()`), which runs the main handler loop.
    exe.addObjectFile(libmicrokit);
    exe.addObject(zig_libmicrokit);
    // Specify the linker script, this is necessary to set the ELF entry point address.
    exe.setLinkerScript(libmicrokit_linker_script);

    exe.linkLibrary(libvmm);

    b.installArtifact(exe);

    const system_description_path = "zig_vmm.system";
    const final_image_dest = b.getInstallPath(.bin, "./loader.img");
    const microkit_tool_cmd = std.Build.Step.Run.create(b, "run microkit tool");
    microkit_tool_cmd.addFileArg(microkit_tool);
    microkit_tool_cmd.addArgs(&[_][]const u8{
       system_description_path,
       "--search-path",
       b.getInstallPath(.bin, ""),
       "--board",
       microkit_board,
       "--config",
       microkit_config,
       "-o",
       final_image_dest,
       "-r",
       b.getInstallPath(.prefix, "./report.txt")
    });
    microkit_tool_cmd.step.dependOn(b.getInstallStep());
    microkit_tool_cmd.setEnvironmentVariable("MICROKIT_SDK", microkit_sdk.getPath3(b, null).toString(b.allocator) catch @panic("OOM"));
    // Add the "microkit" step, and make it the default step when we execute `zig build`
    const microkit_step = b.step("microkit", "Compile and build the final bootable image");
    microkit_step.dependOn(&microkit_tool_cmd.step);
    b.default_step = microkit_step;

    // This is setting up a `qemu` command for running the system using QEMU.
    const loader_arg = b.fmt("loader,file={s},addr=0x70000000,cpu-num=0", .{ final_image_dest });
    const qemu_cmd = b.addSystemCommand(&[_][]const u8{
        "qemu-system-aarch64",
        "-machine",
        "virt,virtualization=on,highmem=off,secure=off",
        "-cpu",
        "cortex-a53",
        "-serial",
        "mon:stdio",
        "-device",
        loader_arg,
        "-m",
        "2G",
        "-nographic",
    });
    qemu_cmd.step.dependOn(b.default_step);
    const simulate_step = b.step("qemu", "Simulate the image using QEMU");
    simulate_step.dependOn(&qemu_cmd.step);
}
