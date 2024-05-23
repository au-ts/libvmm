const std = @import("std");
const Build = std.Build;
const LazyPath = Build.LazyPath;
const Tuple = std.meta.Tuple;
var general_purpose_allocator = std.heap.GeneralPurposeAllocator(.{}){};
const gpa = general_purpose_allocator.allocator();

const MicrokitBoard = enum {
    qemu_arm_virt,
    odroidc4
};

const ConfigOptions = enum {
    debug,
    release,
    benchmark
};

const Target = struct {
    board: MicrokitBoard,
    zig_target: std.Target.Query,
};

const targets = [_]Target {
    .{
        .board = MicrokitBoard.qemu_arm_virt,
        .zig_target = std.Target.Query{
            .cpu_arch = .aarch64,
            .cpu_model = .{ .explicit = &std.Target.arm.cpu.cortex_a53 },
            .os_tag = .freestanding,
            .abi = .none,
        },
    },
    .{
        .board = MicrokitBoard.odroidc4,
        .zig_target = std.Target.Query{
            .cpu_arch = .aarch64,
            .cpu_model = .{ .explicit = &std.Target.arm.cpu.cortex_a55 },
            .os_tag = .freestanding,
            .abi = .none,
        },
    }
};

var libmicrokit: LazyPath = undefined;
var libmicrokit_linker_script: LazyPath = undefined;
var libmicrokit_include: LazyPath = undefined;
var microkit_board_option: MicrokitBoard = undefined;
var libvmm_dep: *Build.Dependency = undefined;
var sddf_dep: *Build.Dependency = undefined;

var dtscat: LazyPath = undefined;
var mkvirtdisk: LazyPath = undefined;
var packrootfs: LazyPath = undefined;

// TODO: Util function, move this elsewhere
fn concatSlice(allocator: std.mem.Allocator, a :[]const u8, b: []const u8) std.mem.Allocator.Error![]const u8 {
    var arrl = std.ArrayList(u8).init(allocator);
    defer arrl.deinit();
    try arrl.appendSlice(a);
    try arrl.appendSlice(b);
    return try arrl.toOwnedSlice();
}

// TODO: Improve error handling to be more generic
fn _concatSliceError(err: std.mem.Allocator.Error) noreturn {
    std.log.err("Failed to concatenate strings: {any}\n", .{err});
    std.posix.exit(1);
}

// TODO: Util function, move this elsewhere
fn fileExists(path: []const u8) bool {
    std.fs.cwd().access(path, .{}) catch return false;
    return true;
}

fn findTarget(board: MicrokitBoard) std.Target.Query {
    for (targets) |target| {
        if (board == target.board) {
            return target.zig_target;
        }
    }

    std.log.err("Board '{}' is not supported\n", .{ board });
    std.posix.exit(1);
}

fn dtscatCmd(
    b: *Build,
    base: LazyPath,
    overlays: []const LazyPath
) Tuple(&.{*Build.Step.Run, LazyPath}) {
    const cmd = Build.Step.Run.create(b, "run dtscat");
    cmd.addFileArg(dtscat);
    cmd.addFileArg(base);
    for (overlays) |overlay| {
        cmd.addFileArg(overlay);
    }
    const output = cmd.captureStdOut();
    return .{cmd, output};
}

fn dtcCmd(
    b: *Build,
    dts: LazyPath
) Tuple(&.{*Build.Step.Run, LazyPath}) {
    const cmd = b.addSystemCommand(&[_][]const u8{
        "dtc", "-q", "-I", "dts", "-O", "dtb"
    });
    cmd.addFileArg(dts);
    const output = cmd.captureStdOut();
    return .{cmd, output};
}

fn packrootfsCmd(   
    b: *Build, 
    rootfs_base: LazyPath,
    rootfs_tmp: []const u8,
    rootfs_name: []const u8,
    home_files: []const LazyPath,
    startup_files: []const LazyPath,
) Tuple(&.{*Build.Step.Run, LazyPath})  
{
    // TODO: Investigate why WF is not working
    // const rootfs_wf = b.addWriteFiles();
    // const output = rootfs_wf.getDirectory().path(b, rootfs_name);
    // const tmp = rootfs_wf.getDirectory().path(b, "tmp");
    const cmd = Build.Step.Run.create(b, "run packrootfs");
    cmd.addFileArg(packrootfs);
    cmd.addFileArg(rootfs_base);
    cmd.addArg(rootfs_tmp);
    cmd.addArg("-o");
    const output = cmd.addOutputFileArg(rootfs_name);
    cmd.addArg("--home");
    for (home_files) |file| {
        cmd.addFileArg(file);
    }
    cmd.addArg("--startup");
    for (startup_files) |file| {
        cmd.addFileArg(file);
    }
    return .{cmd, output};
}

fn mkvirtdiskCmd(
    b: *Build,
    num_part: []const u8,
    logical_size: []const u8,
    blk_mem: []const u8
) Tuple(&.{*Build.Step.Run, LazyPath}) {
    const cmd = Build.Step.Run.create(b, "run mkvirtdisk");
    cmd.addFileArg(mkvirtdisk);
    // TODO: Investigate another way to do this, don't use /tmp?
    const virtdisk = cmd.addOutputFileArg("/tmp/virtdisk");
    cmd.addArg(num_part);
    cmd.addArg(logical_size);
    cmd.addArg(blk_mem);
    return .{cmd, virtdisk};
}

fn addPd(b: *Build, options: Build.ExecutableOptions) *Build.Step.Compile {
    const pd = b.addExecutable(options);
    pd.addObjectFile(libmicrokit);
    pd.setLinkerScriptPath(libmicrokit_linker_script);
    pd.addIncludePath(libmicrokit_include);

    return pd;
}

fn addVmm(
    b: *Build,
    vmm_name: []const u8,
    vmm_src_file: LazyPath,
    rootfs_base: LazyPath,
    rootfs_home: []const LazyPath,
    rootfs_startup: []const LazyPath,
    dts_base: LazyPath,
    dts_overlays: []const LazyPath,
    target: Build.ResolvedTarget,
    optimize: std.builtin.OptimizeMode,
) *Build.Step.Compile {
    const libvmm = libvmm_dep.artifact("vmm");

    const vmm = addPd(b, .{
        .name = b.fmt("{s}_vmm.elf", .{ vmm_name }),
        .target = target,
        .optimize = optimize,
        .strip = false,
    });

    // TODO: Expose only the headers, however can't get the headers to be installed into
    // zig-out .header directory when it is not an artifact/module.
    // vmm.addIncludePath(LazyPath{ .cwd_relative = b.getInstallPath(.header, "sddf") });
    vmm.linkLibrary(sddf_dep.artifact("sddf_util"));
    vmm.linkLibrary(libvmm);

    vmm.addCSourceFile(.{
        .file = vmm_src_file,
        .flags = &.{
            "-Wall",
            "-Werror",
            "-Wno-unused-function",
            "-mstrict-align",
            b.fmt("-DBOARD_{s}", .{ @tagName(microkit_board_option) })
        }
    });

    const vm_name_prefix = concatSlice(gpa, @tagName(microkit_board_option), vmm_name) catch |err| _concatSliceError(err);
    
    // Generate DTB
    const dts_name = concatSlice(gpa, vm_name_prefix, "_vm.dts") catch |err| _concatSliceError(err);
    const dtb_name = concatSlice(gpa, vm_name_prefix, "_vm.dtb") catch |err| _concatSliceError(err);

    _, const dts = dtscatCmd(b, dts_base, dts_overlays);
    const dts_install_step = &b.addInstallFileWithDir(dts, .prefix, dts_name).step;
    
    const dtc_cmd, const dtb = dtcCmd(b, dts);
    dtc_cmd.step.dependOn(dts_install_step);
    const dtb_install_step = &b.addInstallFileWithDir(dtb, .prefix, dtb_name).step;
    
    // Pack rootfs
    const rootfs_name = concatSlice(gpa, vm_name_prefix, "_vm_rootfs.cpio.gz") catch |err| _concatSliceError(err);
    // TODO: Investigate another way to do this, don't use /tmp?
    const rootfs_output_path = b.fmt("/tmp/{s}", .{ rootfs_name });
    const rootfs_tmpdir_path = b.fmt("/tmp/{s}_rootfs", .{ vmm_name });

    // TODO: figure out how to put a directory of files as a dependency. For now
    // we don't check for changes in the files that are packed together
    _, const rootfs = packrootfsCmd(
        b,
        rootfs_base,
        rootfs_output_path,
        rootfs_tmpdir_path,
        rootfs_home,
        rootfs_startup
    );
    const rootfs_install_step = &b.addInstallFileWithDir(rootfs, .prefix, rootfs_name).step;

    const vm_images = b.addObject(.{
        .name = b.fmt("{s}_vm_images", .{ vmm_name }),
        .target = target,
        .optimize = optimize,
    });

    // We need to produce the rootfs before doing anything to produce guest_images
    vm_images.step.dependOn(rootfs_install_step);

    // We need to produce the DTB from the DTS before doing anything to produce guest_images
    vm_images.step.dependOn(dtb_install_step);

    const kernel_image_arg = b.fmt("-DGUEST_KERNEL_IMAGE_PATH=\"{s}\"", .{ b.fmt("board/{s}/{s}_vm/linux", .{ @tagName(microkit_board_option), vmm_name }) });
    const initrd_image_arg = b.fmt("-DGUEST_INITRD_IMAGE_PATH=\"{s}\"", .{ b.getInstallPath(.prefix, rootfs_name) });
    const dtb_image_arg = b.fmt("-DGUEST_DTB_IMAGE_PATH=\"{s}\"", .{ b.getInstallPath(.prefix, dtb_name) });
    vm_images.addCSourceFile(.{
        .file = libvmm_dep.path("tools/package_guest_images.S"),
        .flags = &.{
            kernel_image_arg,
            dtb_image_arg,
            initrd_image_arg,
            "-x",
            "assembler-with-cpp",
        }
    });

    vmm.addObject(vm_images);
    
    return vmm;
}
    
pub fn build(b: *Build) void {
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
    microkit_board_option = microkit_board_option_tmp.?;
    const microkit_board_dir = b.fmt("{s}/board/{s}/{s}", .{ microkit_sdk_option, @tagName(microkit_board_option), @tagName(microkit_config_option) });
    std.fs.cwd().access(microkit_board_dir, .{}) catch |err| {
        switch (err) {
            error.FileNotFound => std.log.err("Path to '{s}' does not exist\n", .{ microkit_board_dir }),
            else => std.log.err("Could not acccess path '{s}', error is {any}\n", .{ microkit_board_dir, err })
        }
        std.posix.exit(1);
    };

    // mkvirtdisk option
    var mkvirtdisk_option: bool = undefined;
    if (microkit_board_option == MicrokitBoard.qemu_arm_virt) {
        mkvirtdisk_option = b.option(bool, "mkvirtdisk", "Create a new virtual disk image") orelse false;
    }

    // Libmicrokit
    const microkit_tool = LazyPath{ .cwd_relative = b.fmt("{s}/bin/microkit", .{ microkit_sdk_option }) };
    libmicrokit = LazyPath{ .cwd_relative = b.fmt("{s}/lib/libmicrokit.a", .{ microkit_board_dir }) };
    libmicrokit_linker_script = LazyPath{ .cwd_relative = b.fmt("{s}/lib/microkit.ld", .{ microkit_board_dir }) };
    libmicrokit_include = LazyPath{ .cwd_relative = b.fmt("{s}/include", .{ microkit_board_dir }) };
    
    // Target
    const target = b.resolveTargetQuery(findTarget(microkit_board_option));
    
    // Dependencies
    libvmm_dep = b.dependency("libvmm", .{
        .target = target,
        .optimize = optimize,
        .sdk = microkit_sdk_option,
        .config = @as([]const u8, @tagName(microkit_config_option)),
        .board = @as([]const u8, @tagName(microkit_board_option)),
    });
    sddf_dep = b.dependency("sddf", .{
        .target = target,
        .optimize = optimize,
        .sdk = microkit_sdk_option,
        .config = @as([]const u8, @tagName(microkit_config_option)),
        .board = @as([]const u8, @tagName(microkit_board_option)),
    });

    // Tools
    // TODO: Expose these tools via a better way?
    dtscat = libvmm_dep.path("tools/dtscat");
    mkvirtdisk = libvmm_dep.path("tools/mkvirtdisk");
    packrootfs = libvmm_dep.path("tools/packrootfs");

    // Blk driver VMM artifact
    const blk_driver_src_file = b.path("blk_driver_vmm.c");
    // // Blk driver VM: Get DTB
    const blk_driver_vm_dts_base = b.path(b.fmt("board/{s}/blk_driver_vm/dts/linux.dts", .{ @tagName(microkit_board_option) }));
    const blk_driver_vm_dts_overlays = &[_]LazyPath{
        b.path(b.fmt("board/{s}/blk_driver_vm/dts/init.dts", .{ @tagName(microkit_board_option) })),
        b.path(b.fmt("board/{s}/blk_driver_vm/dts/io.dts", .{ @tagName(microkit_board_option) })),
        b.path(b.fmt("board/{s}/blk_driver_vm/dts/disable.dts", .{ @tagName(microkit_board_option) })),
    };
    // Blk driver VM: Pack rootfs
    const uio_blk_driver = libvmm_dep.artifact("uio_blk_driver");
    const blk_driver_vm_init = switch (microkit_board_option) {
        MicrokitBoard.qemu_arm_virt => libvmm_dep.path("tools/linux/blk/qemu_blk_driver_init"),
        MicrokitBoard.odroidc4 => libvmm_dep.path("tools/linux/blk/blk_driver_init"),
    };
    const blk_driver_vm_rootfs_base = b.path(b.fmt("board/{s}/blk_driver_vm/rootfs.cpio.gz", .{ @tagName(microkit_board_option) }));
    const blk_driver_vmm = addVmm(
        b, "blk_driver",
        blk_driver_src_file,
        blk_driver_vm_rootfs_base,
        &.{uio_blk_driver.getEmittedBin()},
        &.{blk_driver_vm_init},
        blk_driver_vm_dts_base,
        blk_driver_vm_dts_overlays,
        target,
        optimize
    );
    b.installArtifact(blk_driver_vmm);

    // Client 1 VMM artifact
    const client_src_file = b.path("client_vmm.c");
    // Client 1 VM: Get DTB
    const client_1_vm_dts_base = b.path(b.fmt("board/{s}/client_1_vm/dts/linux.dts", .{ @tagName(microkit_board_option) }));
    const client_1_vm_dts_overlays = &[_]LazyPath{
        b.path(b.fmt("board/{s}/client_1_vm/dts/init.dts", .{ @tagName(microkit_board_option) })),
        b.path(b.fmt("board/{s}/client_1_vm/dts/virtio.dts", .{ @tagName(microkit_board_option) })),
    };
    // Client 1 VM: Pack rootfs
    const client_1_vm_init = switch (microkit_board_option) {
        MicrokitBoard.qemu_arm_virt => libvmm_dep.path("tools/linux/blk/qemu_blk_client_init"),
        MicrokitBoard.odroidc4 => libvmm_dep.path("tools/linux/blk/blk_client_init"),
    };
    const client_1_vm_rootfs_base = b.path(b.fmt("board/{s}/client_1_vm/rootfs.cpio.gz", .{ @tagName(microkit_board_option) }));
    const client_1_vmm = addVmm(
        b, "client_1",
        client_src_file,
        client_1_vm_rootfs_base,
        &.{},
        &.{client_1_vm_init},
        client_1_vm_dts_base,
        client_1_vm_dts_overlays,
        target,
        optimize
    );
    b.installArtifact(client_1_vmm);

    // Client 2 VMM artifact
    // Client 2 VM: Get DTB
    const client_2_vm_dts_base = b.path(b.fmt("board/{s}/client_2_vm/dts/linux.dts", .{ @tagName(microkit_board_option) }));
    const client_2_vm_dts_overlays = &[_]LazyPath{
        b.path(b.fmt("board/{s}/client_2_vm/dts/init.dts", .{ @tagName(microkit_board_option) })),
        b.path(b.fmt("board/{s}/client_2_vm/dts/virtio.dts", .{ @tagName(microkit_board_option) })),
    };
    // Client 2 VM: Pack rootfs
    const client_2_vm_init = switch (microkit_board_option) {
        MicrokitBoard.qemu_arm_virt => libvmm_dep.path("tools/linux/blk/qemu_blk_client_init"),
        MicrokitBoard.odroidc4 => libvmm_dep.path("tools/linux/blk/blk_client_init"),
    };
    const client_2_vm_rootfs_base = b.path(b.fmt("board/{s}/client_2_vm/rootfs.cpio.gz", .{ @tagName(microkit_board_option) }));
    const client_2_vmm = addVmm(
        b, "client_2",
        client_src_file,
        client_2_vm_rootfs_base,
        &.{},
        &.{client_2_vm_init},
        client_2_vm_dts_base,
        client_2_vm_dts_overlays,
        target,
        optimize
    );
    b.installArtifact(client_2_vmm);

    // UART driver artifact
    const uart_driver = switch (microkit_board_option) {
        MicrokitBoard.qemu_arm_virt => sddf_dep.artifact("driver_uart_arm.elf"),
        MicrokitBoard.odroidc4 => sddf_dep.artifact("driver_uart_meson.elf"),
    };
    b.installArtifact(uart_driver);

    // Serial virt TX artifact
    const serial_virt_tx = sddf_dep.artifact("serial_virt_tx.elf");
    b.installArtifact(serial_virt_tx);

    // Serial virt RX artifact
    const serial_virt_rx = sddf_dep.artifact("serial_virt_rx.elf");
    b.installArtifact(serial_virt_rx);

    // Blk virt artifact
    const blk_virt = sddf_dep.artifact("blk_virt.elf");
    b.installArtifact(blk_virt);

    // Microkit tool command
    const system_description_path = LazyPath{ .cwd_relative = b.fmt("board/{s}/virtio.system", .{ @tagName(microkit_board_option) }) };
    const final_image_dest = b.getInstallPath(.bin, "./loader.img");
    const microkit_tool_cmd = b.addSystemCommand(&[_][]const u8{
        microkit_tool.getPath(b),
    });
    microkit_tool_cmd.addFileArg(system_description_path);
    microkit_tool_cmd.addArg("--search-path");
    microkit_tool_cmd.addArg(b.getInstallPath(.bin, ""));
    microkit_tool_cmd.addArg("--board");
    microkit_tool_cmd.addArg(@tagName(microkit_board_option));
    microkit_tool_cmd.addArg("--config");
    microkit_tool_cmd.addArg(@tagName(microkit_config_option));
    microkit_tool_cmd.addArg("-o");
    microkit_tool_cmd.addArg(final_image_dest);
    microkit_tool_cmd.addArg("-r");
    microkit_tool_cmd.addArg(b.getInstallPath(.prefix, "./report.txt"));
    microkit_tool_cmd.step.dependOn(b.getInstallStep());
    // Add the "microkit" step, and make it the default step when we execute `zig build`>
    const microkit_step = b.step("microkit", "Compile and build the final bootable image");
    microkit_step.dependOn(&microkit_tool_cmd.step);
    b.default_step = microkit_step;

    // Make virtual disk
    const virtdisk_name = "storage.img";
    var virtdisk: LazyPath = undefined;
    var made_virtdisk: bool = undefined;
    // Create virtual disk if it doesn't exist or if the user wants to create a new one
    if (mkvirtdisk_option or !fileExists(b.getInstallPath(.prefix, virtdisk_name))) {
        // TODO: Make these configurable options
        const num_part = "2";
        const logical_size = "512";
        const blk_mem = "2101248";
        _, virtdisk = mkvirtdiskCmd(b, num_part, logical_size, blk_mem);
        made_virtdisk = true;
    } else {
        made_virtdisk = false;
    }

    // This is setting up a `qemu` command for running the system using QEMU,
    // which we only want to do when we have a board that we can actually simulate.
    const loader_arg = b.fmt("loader,file={s},addr=0x70000000,cpu-num=0", .{ final_image_dest });
    if (microkit_board_option == MicrokitBoard.qemu_arm_virt) {
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
            "-drive",
            b.fmt("file={s},format=raw,if=none,id=drive0", .{b.getInstallPath(.prefix, virtdisk_name)}),
			"-device",
            "virtio-blk-device,drive=drive0,id=virtblk0,num-queues=1",
        });
        qemu_cmd.step.dependOn(microkit_step);
        if (made_virtdisk) {
            qemu_cmd.step.dependOn(&b.addInstallFileWithDir(virtdisk, .prefix, virtdisk_name).step);
        }
        const simulate_step = b.step("qemu", "Simulate the image using QEMU");
        simulate_step.dependOn(&qemu_cmd.step);
    }
}
