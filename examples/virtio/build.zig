// Copyright 2024, UNSW
// SPDX-License-Identifier: BSD-2-Clause
const std = @import("std");
const LazyPath = std.Build.LazyPath;
const Step = std.Build.Step;

const MicrokitBoard = enum {
    qemu_virt_aarch64,
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

fn updateSectionObjcopy(b: *std.Build, section: []const u8, data_output: std.Build.LazyPath, data: []const u8, elf: []const u8) *std.Build.Step.Run {
    const run_objcopy = b.addSystemCommand(&[_][]const u8{
        "llvm-objcopy",
    });
    run_objcopy.addArg("--update-section");
    const data_full_path = data_output.join(b.allocator, data) catch @panic("OOM");
    run_objcopy.addPrefixedFileArg(b.fmt("{s}=", .{ section }), data_full_path);
    run_objcopy.addFileArg(.{ .cwd_relative = b.getInstallPath(.bin, elf) });

    // We need the ELFs we talk about to be in the install directory first.
    run_objcopy.step.dependOn(b.getInstallStep());

    return run_objcopy;
}

pub fn build(b: *std.Build) !void {
    const optimize = b.standardOptimizeOption(.{});

    const default_python = if (std.posix.getenv("PYTHON")) |p| p else "python3";
    const python = b.option([]const u8, "python", "Path to Python to use") orelse default_python;

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

    const custom_linux = b.option([]const u8, "linux", "Custom Linux image to use") orelse null;
    const custom_initrd = b.option([]const u8, "initrd", "Custom initrd image to use") orelse null;

    const microkit_board_dir = microkit_sdk.path(b, "board").path(b, microkit_board).path(b, microkit_config);
    const microkit_tool = microkit_sdk.path(b, "bin/microkit");
    const libmicrokit = microkit_board_dir.path(b, "lib/libmicrokit.a");
    const libmicrokit_include = microkit_board_dir.path(b, "include");
    const libmicrokit_linker_script = microkit_board_dir.path(b, "lib/microkit.ld");

    const arm_vgic_version: usize = switch (microkit_board_option) {
        .qemu_virt_aarch64 => 2,
        .maaxboard => 3,
    };

    const libvmm_dep = b.dependency("libvmm", .{
        .target = target,
        .optimize = optimize,
        .microkit_board_dir = microkit_board_dir,
        .arm_vgic_version = arm_vgic_version,
    });
    const libvmm = libvmm_dep.artifact("vmm");

    const sddf_dep = b.dependency("sddf", .{
        .target = target,
        .optimize = optimize,
        .microkit_board_dir = microkit_board_dir,
    });

    const client_vmm = b.addExecutable(.{
        .name = "client_vmm.elf",
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            // Microkit expects and requires the symbol table to exist in the ELF,
            // this means that even when building for release mode, we want to tell
            // Zig not to strip symbols from the binary.
            .strip = false,
        }),
    });

    const base_dts_path = "client_vm/linux.dts";
    const overlay = if (arm_vgic_version == 3) "client_vm/gic_v3_overlay.dts" else "client_vm/gic_v2_overlay.dts";
    const dts_cat_cmd = b.addSystemCommand(&[_][]const u8{
        "sh", "../../tools/dtscat", base_dts_path, overlay
    });
    dts_cat_cmd.addFileInput(b.path(base_dts_path));
    dts_cat_cmd.addFileInput(b.path(overlay));
    const final_dts = dts_cat_cmd.captureStdOut();

    // For actually compiling the guest's DTS into a DTB
    const guest_dtc_cmd = b.addSystemCommand(&.{
        "dtc", "-q", "-I", "dts", "-O", "dtb"
    });
    guest_dtc_cmd.addFileArg(.{ .cwd_relative = b.getInstallPath(.prefix, "vm.dts") });
    guest_dtc_cmd.step.dependOn(&b.addInstallFileWithDir(final_dts, .prefix, "vm.dts").step);
    const guest_dtb = guest_dtc_cmd.captureStdOut();

    client_vmm.addIncludePath(libmicrokit_include);
    client_vmm.addObjectFile(libmicrokit);
    client_vmm.setLinkerScript(libmicrokit_linker_script);
    client_vmm.linkLibrary(libvmm);

    client_vmm.addCSourceFiles(.{
        .files = &.{"client_vmm.c"},
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
    guest_images.step.dependOn(&b.addInstallFileWithDir(guest_dtb, .prefix, "linux.dtb").step);

    const linux_image_dep = b.lazyDependency("linux", .{});
    const initrd_image_dep = b.lazyDependency("initrd", .{});

    if (custom_linux) |c| {
        guest_images.step.dependOn(&b.addInstallFileWithDir(.{ .cwd_relative = c }, .prefix, "linux").step);
    } else if (linux_image_dep) |linux_image| {
        guest_images.step.dependOn(&b.addInstallFileWithDir(linux_image.path("linux"), .prefix, "linux").step);
    }

    // This is a bit weird but since we are making use of lazy dependencies when Zig evaluates
    // build.zig to check for lazy dependencies it means that we could have a null initrd.
    const initrd: ?LazyPath = blk: {
        if (custom_initrd) |p| {
            break :blk b.path(p);
        } else if (initrd_image_dep) |p| {
            break :blk p.path("rootfs.cpio.gz");
        } else {
            break :blk null;
        }
    };

    // This steps adds any extra scripts we need as part of the initrd that are not
    // already packaged into the CPIO archive.
    if (initrd != null) {
        const packrootfs_cmd = b.addSystemCommand(&.{
            "bash"
        });
        const init_scripts = .{
            libvmm_dep.path("tools/linux/blk/blk_client_init"),
            libvmm_dep.path("tools/linux/net/net_client_init"),
        };
        const packrootfs = libvmm_dep.path("tools/packrootfs");
        packrootfs_cmd.addFileArg(packrootfs);
        packrootfs_cmd.addFileInput(packrootfs);
        packrootfs_cmd.addFileArg(initrd.?);
        _ = packrootfs_cmd.addOutputDirectoryArg("rootfs_staging");
        packrootfs_cmd.addArg("-o");
        const packed_rootfs = packrootfs_cmd.addOutputFileArg("rootfs.cpio.gz");
        packrootfs_cmd.addArg("--startup");
        inline for (init_scripts) |s| {
            packrootfs_cmd.addFileInput(s);
            packrootfs_cmd.addFileArg(s);
        }
        guest_images.step.dependOn(&b.addInstallFileWithDir(packed_rootfs, .prefix, "rootfs.cpio.gz").step);
    }

    // Hack to avoid caching of the guest images incbins: https://github.com/ziglang/zig/issues/16919
    var prng = std.Random.DefaultPrng.init(blk: {
        var seed: u64 = undefined;
        try std.posix.getrandom(std.mem.asBytes(&seed));
        break :blk seed;
    });
    const rand = prng.random();
    const random_arg = b.fmt("-DRANDOM=\"{}\"", .{ rand.int(usize) });

    const kernel_image_arg = b.fmt("-DGUEST_KERNEL_IMAGE_PATH=\"{s}\"", .{ b.getInstallPath(.prefix, "linux") });
    const initrd_image_arg = b.fmt("-DGUEST_INITRD_IMAGE_PATH=\"{s}\"", .{ b.getInstallPath(.prefix, "rootfs.cpio.gz") });
    const dtb_image_arg = b.fmt("-DGUEST_DTB_IMAGE_PATH=\"{s}\"", .{ b.getInstallPath(.prefix, "linux.dtb") });
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

    client_vmm.addObject(guest_images);
    b.installArtifact(client_vmm);

    const timer_driver_class = switch (microkit_board_option) {
        .maaxboard => "imx",
        else => null,
    };
    var timer_driver_install: ?*Step.InstallArtifact = null;
    if (timer_driver_class) |c| {
        const driver = sddf_dep.artifact(b.fmt("driver_timer_{s}.elf", .{ c }));
        // To match what our SDF expects
        timer_driver_install = b.addInstallArtifact(driver, .{ .dest_sub_path = "timer_driver.elf" });
    }

    const blk_driver_class = switch (microkit_board_option) {
        .qemu_virt_aarch64 => "virtio",
        .maaxboard => "mmc_imx",
    };

    const serial_driver_class = switch (microkit_board_option) {
        .qemu_virt_aarch64 => "arm",
        .maaxboard => "imx",
    };

    const eth_driver_class = switch (microkit_board_option) {
        .qemu_virt_aarch64 => "virtio",
        .maaxboard => "imx",
    };

    const blk_driver = sddf_dep.artifact(b.fmt("driver_blk_{s}.elf", .{ blk_driver_class }));
    const serial_driver = sddf_dep.artifact(b.fmt("driver_serial_{s}.elf", .{ serial_driver_class }));
    const eth_driver = sddf_dep.artifact(b.fmt("driver_net_{s}.elf", .{ eth_driver_class }));

    b.installArtifact(blk_driver);
    b.installArtifact(sddf_dep.artifact("blk_virt.elf"));
    b.installArtifact(sddf_dep.artifact("serial_virt_rx.elf"));
    b.installArtifact(sddf_dep.artifact("serial_virt_tx.elf"));
    const net_virt_rx = sddf_dep.artifact("net_virt_rx.elf");
    const net_virt_tx = sddf_dep.artifact("net_virt_tx.elf");
    const net_copy = sddf_dep.artifact("net_copy.elf");
    // Because our SDF expects a different ELF name for these articats, we have some extra steps.
    const blk_driver_install = b.addInstallArtifact(blk_driver, .{ .dest_sub_path = "blk_driver.elf" });
    const serial_driver_install = b.addInstallArtifact(serial_driver, .{ .dest_sub_path = "serial_driver.elf" });
    const eth_driver_install = b.addInstallArtifact(eth_driver, .{ .dest_sub_path = "eth_driver.elf" });

    const net_virt_rx_install = b.addInstallArtifact(net_virt_rx, .{ .dest_sub_path = "network_virt_rx.elf" });
    const net_virt_tx_install = b.addInstallArtifact(net_virt_tx, .{ .dest_sub_path = "network_virt_tx.elf" });
    const net_copy_install = b.addInstallArtifact(net_copy, .{ .dest_sub_path = "network_copy.elf" });

    // For compiling the metaprogram needed DTS into a DTB
    const dts = sddf_dep.path(b.fmt("dts/{s}.dts", .{ microkit_board }));
    const dtc_cmd = b.addSystemCommand(&[_][]const u8{
        "dtc", "-q", "-I", "dts", "-O", "dtb"
    });
    dtc_cmd.addFileInput(dts);
    dtc_cmd.addFileArg(dts);
    const dtb = dtc_cmd.captureStdOut();

    // Run the metaprogram to get sDDF configuration binary files and the SDF file.
    const metaprogram = b.path("meta.py");
    const run_metaprogram = b.addSystemCommand(&[_][]const u8{
        python,
    });
    run_metaprogram.addFileArg(metaprogram);
    run_metaprogram.addFileInput(metaprogram);
    run_metaprogram.addPrefixedDirectoryArg("--sddf=", sddf_dep.path(""));
    run_metaprogram.addPrefixedDirectoryArg("--dtb=", dtb);
    run_metaprogram.addPrefixedDirectoryArg("--client-dtb=", guest_dtb);
    const meta_output = run_metaprogram.addPrefixedOutputDirectoryArg("--output=", "meta_output");
    run_metaprogram.addArg("--board");
    run_metaprogram.addArg(microkit_board);
    run_metaprogram.addArg("--sdf");
    run_metaprogram.addArg("virtio.system");

    const meta_output_install = b.addInstallDirectory(.{
        .source_dir = meta_output,
        .install_dir = .prefix,
        .install_subdir = "meta_output",
    });

    // Build up objcopys
    var objcopys = std.ArrayList(*Step.Run).init(b.allocator);
    // Block
    {
        const virt_objcopy = updateSectionObjcopy(b, ".blk_virt_config", meta_output, "blk_virt.data", "blk_virt.elf");
        const driver_resources_objcopy = updateSectionObjcopy(b, ".device_resources", meta_output, "blk_driver_device_resources.data", "blk_driver.elf");
        const driver_config_objcopy = updateSectionObjcopy(b, ".blk_driver_config", meta_output, "blk_driver.data", "blk_driver.elf");
        driver_resources_objcopy.step.dependOn(&blk_driver_install.step);
        driver_config_objcopy.step.dependOn(&blk_driver_install.step);

        try objcopys.append(virt_objcopy);
        try objcopys.append(driver_resources_objcopy);
        try objcopys.append(driver_config_objcopy);

        const client_objcopy = updateSectionObjcopy(b, ".blk_client_config", meta_output, "blk_client_CLIENT_VMM.data", client_vmm.name);
        client_objcopy.step.dependOn(&client_vmm.step);
        try objcopys.append(client_objcopy);
    }
    // Serial
    {
        const driver_resources_objcopy = updateSectionObjcopy(b, ".device_resources", meta_output, "serial_driver_device_resources.data", "serial_driver.elf");
        driver_resources_objcopy.step.dependOn(&serial_driver_install.step);
        const driver_config_objcopy = updateSectionObjcopy(b, ".serial_driver_config", meta_output, "serial_driver_config.data", "serial_driver.elf");
        driver_config_objcopy.step.dependOn(&serial_driver_install.step);
        const virt_rx_objcopy = updateSectionObjcopy(b, ".serial_virt_rx_config", meta_output, "serial_virt_rx.data", "serial_virt_rx.elf");
        const virt_tx_objcopy = updateSectionObjcopy(b, ".serial_virt_tx_config", meta_output, "serial_virt_tx.data", "serial_virt_tx.elf");

        try objcopys.append(virt_rx_objcopy);
        try objcopys.append(virt_tx_objcopy);
        try objcopys.append(driver_resources_objcopy);
        try objcopys.append(driver_config_objcopy);

        const client_objcopy = updateSectionObjcopy(b, ".serial_client_config", meta_output, "serial_client_CLIENT_VMM.data", client_vmm.name);
        client_objcopy.step.dependOn(&client_vmm.step);
        try objcopys.append(client_objcopy);
    }
    // Network
    {
        const driver_resources_objcopy = updateSectionObjcopy(b, ".device_resources", meta_output, "eth_driver_device_resources.data", "eth_driver.elf");
        driver_resources_objcopy.step.dependOn(&eth_driver_install.step);
        const driver_config_objcopy = updateSectionObjcopy(b, ".net_driver_config", meta_output, "net_driver.data", "eth_driver.elf");
        driver_config_objcopy.step.dependOn(&eth_driver_install.step);
        const virt_rx_objcopy = updateSectionObjcopy(b, ".net_virt_rx_config", meta_output, "net_virt_rx.data", "network_virt_rx.elf");
        virt_rx_objcopy.step.dependOn(&net_virt_rx_install.step);
        const virt_tx_objcopy = updateSectionObjcopy(b, ".net_virt_tx_config", meta_output, "net_virt_tx.data", "network_virt_tx.elf");
        virt_tx_objcopy.step.dependOn(&net_virt_tx_install.step);
        const copy_rx_objcopy = updateSectionObjcopy(b, ".net_copy_config", meta_output, "net_copy_client0_net_copier.data", "network_copy.elf");
        copy_rx_objcopy.step.dependOn(&net_copy_install.step);

        try objcopys.append(copy_rx_objcopy);
        try objcopys.append(virt_rx_objcopy);
        try objcopys.append(virt_tx_objcopy);
        try objcopys.append(driver_resources_objcopy);
        try objcopys.append(driver_config_objcopy);

        const client_objcopy = updateSectionObjcopy(b, ".net_client_config", meta_output, "net_client_CLIENT_VMM.data", client_vmm.name);
        client_objcopy.step.dependOn(&client_vmm.step);
        try objcopys.append(client_objcopy);
    }
    // Timer
    {
        if (timer_driver_install != null) {
            const blk_driver_timer_objcopy = updateSectionObjcopy(b, ".timer_client_config", meta_output, "timer_client_blk_driver.data", "blk_driver.elf");
            blk_driver_timer_objcopy.step.dependOn(&blk_driver_install.step);
            const timer_driver_objcopy = updateSectionObjcopy(b, ".device_resources", meta_output, "timer_driver_device_resources.data", "timer_driver.elf");
            timer_driver_objcopy.step.dependOn(&timer_driver_install.?.step);

            try objcopys.append(blk_driver_timer_objcopy);
            try objcopys.append(timer_driver_objcopy);
        }
    }
    // VMM
    {
        const vmm_objcopy = updateSectionObjcopy(b, ".vmm_config", meta_output, "vmm_CLIENT_VMM.data", "client_vmm.elf");
        try objcopys.append(vmm_objcopy);
    }

    const final_image_dest = b.getInstallPath(.bin, "./loader.img");
    const microkit_tool_cmd = Step.Run.create(b, "run microkit tool");
    microkit_tool_cmd.addFileArg(microkit_tool);
    microkit_tool_cmd.addArgs(&[_][]const u8{
        b.getInstallPath(.{ .custom = "meta_output" }, "virtio.system"),
        "--search-path", b.getInstallPath(.bin, ""),
        "--board", microkit_board,
        "--config", microkit_config,
        "-o", final_image_dest,
        "-r", b.getInstallPath(.prefix, "./report.txt")
    });
    for (objcopys.items) |objcopy| {
        microkit_tool_cmd.step.dependOn(&objcopy.step);
    }
    microkit_tool_cmd.step.dependOn(&meta_output_install.step);
    microkit_tool_cmd.step.dependOn(b.getInstallStep());
    microkit_tool_cmd.setEnvironmentVariable("MICROKIT_SDK", microkit_sdk.getPath3(b, null).toString(b.allocator) catch @panic("OOM"));
    // Add the "microkit" step, and make it the default step when we execute `zig build`>
    const microkit_step = b.step("microkit", "Compile and build the final bootable image");
    microkit_step.dependOn(&microkit_tool_cmd.step);
    b.default_step = microkit_step;

    // This is setting up a `qemu` command for running the system using QEMU,
    // which we only want to do when we have a board that we can actually simulate.
    var qemu_cmd: ?*Step.Run = null;
    if (microkit_board_option == .qemu_virt_aarch64) {
        const loader_arg = b.fmt("loader,file={s},addr=0x70000000,cpu-num=0", .{final_image_dest});
        qemu_cmd = b.addSystemCommand(&[_][]const u8{
            "qemu-system-aarch64",
            "-machine",
            "virt,virtualization=on",
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
    }

    if (qemu_cmd) |cmd| {
        const create_disk_cmd = b.addSystemCommand(&[_][]const u8{
            "bash",
        });
        const mkvirtdisk = sddf_dep.path("tools/mkvirtdisk");
        create_disk_cmd.addFileArg(mkvirtdisk);
        create_disk_cmd.addFileInput(mkvirtdisk);
        const disk = create_disk_cmd.addOutputFileArg("disk");
        create_disk_cmd.addArgs(&[_][]const u8{
            "1", "512", b.fmt("{}", .{ 1024 * 1024 * 16 }), "GPT",
        });

        // We want changes to the virtual disk to persist, so only install it if
        // it doesn't already exist.
        std.fs.cwd().access(b.getInstallPath(.prefix, "disk"), .{}) catch {
            const disk_install = b.addInstallFile(disk, "disk");
            disk_install.step.dependOn(&create_disk_cmd.step);
            cmd.step.dependOn(&disk_install.step);
        };

        cmd.addArgs(&.{
            "-global", "virtio-mmio.force-legacy=false",
            "-drive", b.fmt("file={s},format=raw,if=none,id=drive0", .{ b.getInstallPath(.prefix, "disk") }),
            "-device", "virtio-blk-device,drive=drive0,id=virtblk0,num-queues=1",
            "-device", "virtio-net-device,netdev=netdev0",
            "-netdev", "user,id=netdev0,hostfwd=tcp::1236-:1236,hostfwd=tcp::1237-:1237,hostfwd=udp::1235-:1235",
        });

        cmd.step.dependOn(b.default_step);
        const simulate_step = b.step("qemu", "Simulate the image using QEMU");
        simulate_step.dependOn(&cmd.step);
    }
}
