// Copyright 2024, UNSW
// SPDX-License-Identifier: BSD-2-Clause
const std = @import("std");
const LazyPath = std.Build.LazyPath;

const MicrokitBoard = enum {
    qemu_virt_aarch64,
    qemu_virt_riscv64,
    hifive_p550,
    odroidc4,
    maaxboard,
};

const Target = struct {
    board: MicrokitBoard,
    zig_target: std.Target.Query,
};

const targets = [_]Target {
    .{
        .board = MicrokitBoard.qemu_virt_aarch64,
        .zig_target = std.Target.Query{
            .cpu_arch = .aarch64,
            .cpu_model = .{ .explicit = &std.Target.aarch64.cpu.cortex_a53 },
            .cpu_features_add = std.Target.aarch64.featureSet(&[_]std.Target.aarch64.Feature{ .strict_align }),
            .os_tag = .freestanding,
            .abi = .none,
        },
    },
   .{
        .board = MicrokitBoard.qemu_virt_riscv64,
        .zig_target = std.Target.Query{
            .cpu_arch = .riscv64,
            .cpu_model = .{ .explicit = &std.Target.riscv.cpu.baseline_rv64 },
            .os_tag = .freestanding,
            .abi = .none,
        },
    },
    .{
        .board = MicrokitBoard.hifive_p550,
        .zig_target = std.Target.Query{
            .cpu_arch = .riscv64,
            .cpu_model = .{ .explicit = &std.Target.riscv.cpu.baseline_rv64 },
            .os_tag = .freestanding,
            .abi = .none,
        },
    },
    .{
        .board = MicrokitBoard.odroidc4,
        .zig_target = std.Target.Query{
            .cpu_arch = .aarch64,
            .cpu_model = .{ .explicit = &std.Target.aarch64.cpu.cortex_a55 },
            .cpu_features_add = std.Target.aarch64.featureSet(&[_]std.Target.aarch64.Feature{ .strict_align }),
            .os_tag = .freestanding,
            .abi = .none,
        },
    },
    .{
        .board = MicrokitBoard.maaxboard,
        .zig_target = std.Target.Query{
            .cpu_arch = .aarch64,
            .cpu_model = .{ .explicit = &std.Target.aarch64.cpu.cortex_a53 },
            .cpu_features_add = std.Target.aarch64.featureSet(&[_]std.Target.aarch64.Feature{ .strict_align }),
            .os_tag = .freestanding,
            .abi = .none,
        },
    },
};

fn findTarget(board: MicrokitBoard) std.Target.Query {
    for (targets) |target| {
        if (board == target.board) {
            return target.zig_target;
        }
    }

    std.log.err("Board '{}' is not supported\n", .{ board });
    std.posix.exit(1);
}

const ConfigOptions = enum {
    debug,
    release,
    benchmark
};

pub fn build(b: *std.Build) !void {
    const optimize = b.standardOptimizeOption(.{});
    const microkit_sdk = b.option(LazyPath, "sdk", "Path to Microkit SDK") orelse {
        std.log.err("Missing -Dsdk=/path/to/sdk argument", .{});
        return error.MissingMicrokitSdk;
    };

    const microkit_config_option = b.option(ConfigOptions, "config", "Microkit config to build for") orelse ConfigOptions.debug;
    const microkit_config = @tagName(microkit_config_option);

    // Get the Microkit SDK board we want to target
    const microkit_board_option = b.option(MicrokitBoard, "board", "Microkit board to target") orelse {
        std.log.err("Missing -Dboard=<board> argument", .{});
        return error.MissingBoard;
    };
    const target = b.resolveTargetQuery(findTarget(microkit_board_option));
    const microkit_board = @tagName(microkit_board_option);

    const custom_linux = b.option(std.Build.LazyPath, "linux", "Custom Linux image to use") orelse null;
    const custom_initrd = b.option(std.Build.LazyPath, "initrd", "Custom initrd image to use") orelse null;

    const microkit_board_dir = microkit_sdk.path(b, "board").path(b, microkit_board).path(b, microkit_config);
    const microkit_tool = microkit_sdk.path(b, "bin/microkit");
    const libmicrokit = microkit_board_dir.path(b, "lib/libmicrokit.a");
    const libmicrokit_include = microkit_board_dir.path(b, "include");
    const libmicrokit_linker_script = microkit_board_dir.path(b, "lib/microkit.ld");

    const arm_vgic_version: usize = switch (microkit_board_option) {
        .qemu_virt_aarch64, .odroidc4 => 2,
        .maaxboard => 3,
        .qemu_virt_riscv64, .hifive_p550 => 0,
    };

    const libvmm_dep = b.dependency("libvmm", .{
        .target = target,
        .optimize = optimize,
        .microkit_board_dir = microkit_board_dir,
        .arm_vgic_version = arm_vgic_version,
    });
    const libvmm = libvmm_dep.artifact("vmm");

    const exe = b.addExecutable(.{
        .name = "vmm.elf",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            // Microkit expects and requires the symbol table to exist in the ELF,
            // this means that even when building for release mode, we want to tell
            // Zig not to strip symbols from the binary.
            .strip = false,
        }),
    });

    const base_dts_path = b.fmt("board/{s}/linux.dts", .{ microkit_board });
    const overlay = b.fmt("board/{s}/overlay.dts", .{ microkit_board });
    const dts_cat_cmd = b.addSystemCommand(&[_][]const u8{
        "sh", "../../tools/dtscat", base_dts_path, overlay
    });
    dts_cat_cmd.addFileInput(b.path(base_dts_path));
    dts_cat_cmd.addFileInput(b.path(overlay));
    const final_dts = dts_cat_cmd.captureStdOut();

    // For actually compiling the DTS into a DTB
    const dtc_cmd = b.addSystemCommand(&[_][]const u8{
        "dtc", "-q", "-I", "dts", "-O", "dtb"
    });
    dtc_cmd.addFileArg(.{ .cwd_relative = b.getInstallPath(.prefix, "final.dts") });
    dtc_cmd.step.dependOn(&b.addInstallFileWithDir(final_dts, .prefix, "final.dts").step);
    const dtb = dtc_cmd.captureStdOut();

    // Add microkit.h to be used by the API wrapper.
    exe.addIncludePath(libmicrokit_include);
    // Add the static library that provides each protection domain's entry
    // point (`main()`), which runs the main handler loop.
    exe.addObjectFile(libmicrokit);
    // Specify the linker script, this is necessary to set the ELF entry point address.
    exe.setLinkerScript(libmicrokit_linker_script);

    // Link the libvmm library, this will automatically add the libvmm headers as well.
    exe.linkLibrary(libvmm);

    exe.addCSourceFiles(.{
        .files = &.{"vmm.c"},
        .flags = &.{
            "-Wall",
            "-Werror",
            "-Wno-unused-function",
            "-mstrict-align",
            b.fmt("-DBOARD_{s}", .{ microkit_board })
        }
    });

    const guest_images = b.addObject(.{
        .name = "guest_images",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
        }),
    });
    // We need to produce the DTB from the DTS before doing anything to produce guest_images
    guest_images.step.dependOn(&b.addInstallFileWithDir(dtb, .prefix, "linux.dtb").step);

    if (custom_linux) |c| {
        guest_images.step.dependOn(&b.addInstallFileWithDir(c, .prefix, "linux").step);
    } else if (target.result.cpu.arch == .riscv64) {
        guest_images.step.dependOn(&b.addInstallFileWithDir(b.path("board/qemu_virt_riscv64/linux"), .prefix, "linux").step);
    } else {
        const linux_image_dep = b.lazyDependency("linux", .{});
        if (linux_image_dep) |linux_image| {
            guest_images.step.dependOn(&b.addInstallFileWithDir(linux_image.path("linux"), .prefix, "linux").step);
        }
    }

    if (custom_initrd) |c| {
        guest_images.step.dependOn(&b.addInstallFileWithDir(c, .prefix, "rootfs.cpio.gz").step);
    } else if (target.result.cpu.arch == .riscv64) {
        guest_images.step.dependOn(&b.addInstallFileWithDir(b.path("board/qemu_virt_riscv64/rootfs.cpio.gz"), .prefix, "rootfs.cpio.gz").step);
    } else {
        const initrd_image_dep = b.lazyDependency(b.fmt("{s}_initrd", .{ microkit_board }), .{});
        if (initrd_image_dep) |initrd_image| {
            guest_images.step.dependOn(&b.addInstallFileWithDir(initrd_image.path("rootfs.cpio.gz"), .prefix, "rootfs.cpio.gz").step);
        }
    }

    const kernel_image_arg = b.fmt("-DGUEST_KERNEL_IMAGE_PATH=\"{s}\"", .{ b.getInstallPath(.prefix, "linux") });
    const initrd_image_arg = b.fmt("-DGUEST_INITRD_IMAGE_PATH=\"{s}\"", .{ b.getInstallPath(.prefix, "rootfs.cpio.gz") });

    // Hack to avoid caching of the guest images incbins: https://github.com/ziglang/zig/issues/16919
    var prng = std.Random.DefaultPrng.init(blk: {
        var seed: u64 = undefined;
        try std.posix.getrandom(std.mem.asBytes(&seed));
        break :blk seed;
    });
    const rand = prng.random();

    const dtb_image_arg = b.fmt("-DGUEST_DTB_IMAGE_PATH=\"{s}\"", .{ b.getInstallPath(.prefix, "linux.dtb") });
    const random_arg = b.fmt("-DRANDOM=\"{}\"", .{ rand.int(usize) });
    guest_images.addCSourceFile(.{
        .file = libvmm_dep.path("tools/package_guest_images.S"),
        .flags = &.{
            kernel_image_arg,
            dtb_image_arg,
            initrd_image_arg,
            random_arg,
            "-x",
            "assembler-with-cpp",
        }
    });

    exe.addObject(guest_images);
    b.installArtifact(exe);

    const system_description_path = b.fmt("board/{s}/simple.system", .{ microkit_board });
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

    // This is setting up a `qemu` command for running the system using QEMU,
    // which we only want to do when we have a board that we can actually simulate.
    const loader_arg = b.fmt("loader,file={s},addr=0x70000000,cpu-num=0", .{ final_image_dest });
    if (microkit_board_option == .qemu_virt_aarch64) {
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
    } else if (microkit_board_option == .qemu_virt_riscv64) {
        const qemu_cmd = b.addSystemCommand(&[_][]const u8{
            "qemu-system-riscv64",
            "-machine",
            "virt",
            "-serial",
            "mon:stdio",
            "-kernel",
            final_image_dest,
            "-m",
            "2G",
            "-nographic",
        });
        qemu_cmd.step.dependOn(b.default_step);
        const simulate_step = b.step("qemu", "Simulate the image via QEMU");
        simulate_step.dependOn(&qemu_cmd.step);
    }
}
