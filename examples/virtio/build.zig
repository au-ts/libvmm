const std = @import("std");
const Build = std.Build;
const LazyPath = Build.LazyPath;
const Tuple = std.meta.Tuple;

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
            .ofmt = .elf,
        },
    },
    .{
        .board = MicrokitBoard.odroidc4,
        .zig_target = std.Target.Query{
            .cpu_arch = .aarch64,
            .cpu_model = .{ .explicit = &std.Target.arm.cpu.cortex_a55 },
            .os_tag = .freestanding,
            .abi = .none,
            .ofmt = .elf,
        },
    }
};

var libmicrokit: LazyPath = undefined;
var libmicrokit_linker_script: LazyPath = undefined;
var libmicrokit_include: LazyPath = undefined;
var microkit_board: MicrokitBoard = undefined;
var libvmm_dep: *Build.Dependency = undefined;
var sddf_dep: *Build.Dependency = undefined;

var dtscat: LazyPath = undefined;
var mkvirtdisk: LazyPath = undefined;
var packrootfs: LazyPath = undefined;

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
    vm_name: []const u8,
    prog_img: []const u8,
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
        .name = prog_img,
        .target = target,
        .optimize = optimize,
        .strip = false,
    });

    vmm.linkLibrary(libvmm);
    vmm.addIncludePath(sddf_dep.path("include"));
    vmm.addIncludePath(b.path("include"));

    vmm.addCSourceFile(.{
        .file = vmm_src_file,
        .flags = &.{
            "-Wall",
            "-Werror",
            "-Wno-unused-function",
            "-mstrict-align",
            b.fmt("-DBOARD_{s}", .{ @tagName(microkit_board) })
        }
    });

    // Generate DTB
    const dts_name = b.fmt("{s}_{s}.dts", .{@tagName(microkit_board), vm_name});
    const dtb_name = b.fmt("{s}_{s}.dtb", .{@tagName(microkit_board), vm_name});

    _, const dts = dtscatCmd(b, dts_base, dts_overlays);
    const dts_install_step = &b.addInstallFileWithDir(dts, .prefix, dts_name).step;

    const dtc_cmd, const dtb = dtcCmd(b, dts);
    dtc_cmd.step.dependOn(dts_install_step);
    const dtb_install_step = &b.addInstallFileWithDir(dtb, .prefix, dtb_name).step;

    // Pack rootfs
    const rootfs_name = b.fmt("{s}_{s}_rootfs.cpio.gz", .{@tagName(microkit_board), vm_name});
    // TODO: Investigate another way to do this, don't use /tmp?
    const rootfs_tmpdir_path = b.fmt("/tmp/{s}_{s}_rootfs", .{ @tagName(microkit_board), vm_name});
    const rootfs_output_path = b.fmt("/tmp/{s}", .{ rootfs_name });

    // TODO: figure out how to put a directory of files as a dependency. For now
    // we don't check for changes in the files that are packed together
    _, const rootfs = packrootfsCmd(
        b,
        rootfs_base,
        rootfs_tmpdir_path,
        rootfs_output_path,
        rootfs_home,
        rootfs_startup
    );
    const rootfs_install_step = &b.addInstallFileWithDir(rootfs, .prefix, rootfs_name).step;

    const vm_images = b.addObject(.{
        .name = b.fmt("{s}_images.o", .{ vm_name }),
        .target = target,
        .optimize = optimize,
    });
    // We need to produce the rootfs before doing anything to produce guest_images
    vm_images.step.dependOn(rootfs_install_step);
    // We need to produce the DTB from the DTS before doing anything to produce guest_images
    vm_images.step.dependOn(dtb_install_step);

    const kernel_image_arg = b.fmt("-DGUEST_KERNEL_IMAGE_PATH=\"{s}\"", .{ b.fmt("board/{s}/{s}/linux", .{ @tagName(microkit_board), vm_name }) });
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
    const microkit_sdk_option = b.option([]const u8, "sdk", "Path to Microkit SDK");
    if (microkit_sdk_option == null) {
        std.log.err("Missing -Dsdk=/path/to/sdk argument being passed\n", .{});
        std.posix.exit(1);
    }
    const microkit_sdk = microkit_sdk_option.?;
    std.fs.cwd().access(microkit_sdk, .{}) catch |err| {
        switch (err) {
            error.FileNotFound => std.log.err("Path to SDK '{s}' does not exist\n", .{ microkit_sdk }),
            else => std.log.err("Could not acccess SDK path '{s}', error is {any}\n", .{ microkit_sdk, err })
        }
        std.posix.exit(1);
    };

    // Microkit config option
    const microkit_config = b.option(ConfigOptions, "config", "Microkit config to build for") orelse ConfigOptions.debug;

    // Microkit board option
    const microkit_board_option = b.option(MicrokitBoard, "board", "Microkit board to target");
    if (microkit_board_option == null) {
        std.log.err("Missing -Dboard=<BOARD> argument being passed\n", .{});
        std.posix.exit(1);
    }
    microkit_board = microkit_board_option.?;
    const microkit_board_dir = b.fmt("{s}/board/{s}/{s}", .{ microkit_sdk, @tagName(microkit_board), @tagName(microkit_config) });
    std.fs.cwd().access(microkit_board_dir, .{}) catch |err| {
        switch (err) {
            error.FileNotFound => std.log.err("Path to '{s}' does not exist\n", .{ microkit_board_dir }),
            else => std.log.err("Could not acccess path '{s}', error is {any}\n", .{ microkit_board_dir, err })
        }
        std.posix.exit(1);
    };

    // mkvirtdisk option
    var mkvirtdisk_option: bool = undefined;
    if (microkit_board == MicrokitBoard.qemu_arm_virt) {
        mkvirtdisk_option = b.option(bool, "mkvirtdisk", "Create a new virtual disk image") orelse false;
    }

    // Libmicrokit
    const libmicrokit_path = b.fmt("{s}/lib/libmicrokit.a", .{ microkit_board_dir });
    const libmicrokit_linker_script_path = b.fmt("{s}/lib/microkit.ld", .{ microkit_board_dir });
    const libmicrokit_include_path = b.fmt("{s}/include", .{ microkit_board_dir });
    const microkit_tool = LazyPath{ .cwd_relative = b.fmt("{s}/bin/microkit", .{ microkit_sdk }) };
    libmicrokit = LazyPath{ .cwd_relative = libmicrokit_path };
    libmicrokit_linker_script = LazyPath{ .cwd_relative = libmicrokit_linker_script_path };
    libmicrokit_include = LazyPath{ .cwd_relative = libmicrokit_include_path };
    
    // Target
    const target = b.resolveTargetQuery(findTarget(microkit_board));
    
    // Dependencies
    libvmm_dep = b.dependency("libvmm", .{
        .target = target,
        .optimize = optimize,
        .libmicrokit = @as([]const u8, libmicrokit_path),
        .libmicrokit_include = @as([]const u8, libmicrokit_include_path),
        .libmicrokit_linker_script = @as([]const u8, libmicrokit_linker_script_path),
        .microkit_board = @as([]const u8, @tagName(microkit_board)),
        .arm_vgic_version = @as(usize, 2),
        .blk_config_include = @as([]const u8, "include"),
    });
    sddf_dep = b.dependency("sddf", .{
        .target = target,
        .optimize = optimize,
        .libmicrokit = @as([]const u8, libmicrokit_path),
        .libmicrokit_include = @as([]const u8, libmicrokit_include_path),
        .libmicrokit_linker_script = @as([]const u8, libmicrokit_linker_script_path),
        .blk_config_include = @as([]const u8, "include"),
        .serial_config_include = @as([]const u8, "include")
    });

    // Tools
    // TODO: Expose these tools via a better way?
    dtscat = libvmm_dep.path("tools/dtscat");
    mkvirtdisk = libvmm_dep.path("tools/mkvirtdisk");
    packrootfs = libvmm_dep.path("tools/packrootfs");

    // Blk driver VMM artifact
    const blk_driver_src_file = b.path("blk_driver_vmm.c");
    // Blk driver VM: Get DTB
    const blk_driver_vm_dts_base = b.path(b.fmt("board/{s}/blk_driver_vm/dts/linux.dts", .{ @tagName(microkit_board) }));
    const blk_driver_vm_dts_overlays = &[_]LazyPath{
        b.path(b.fmt("board/{s}/blk_driver_vm/dts/init.dts", .{ @tagName(microkit_board) })),
        b.path(b.fmt("board/{s}/blk_driver_vm/dts/io.dts", .{ @tagName(microkit_board) })),
        b.path(b.fmt("board/{s}/blk_driver_vm/dts/disable.dts", .{ @tagName(microkit_board) })),
    };
    // Blk driver VM: Pack rootfs
    const uio_driver_blk = libvmm_dep.artifact("uio_driver_blk");
    const blk_driver_vm_init = libvmm_dep.path(b.fmt("tools/linux/blk/board/{s}/blk_driver_init", .{ @tagName(microkit_board) }));
    const blk_driver_vm_rootfs_base = b.path(b.fmt("board/{s}/blk_driver_vm/rootfs.cpio.gz", .{ @tagName(microkit_board) }));
    const blk_driver_vmm = addVmm(
        b,
        "blk_driver_vm",
        "blk_driver_vmm.elf",
        blk_driver_src_file,
        blk_driver_vm_rootfs_base,
        &.{uio_driver_blk.getEmittedBin()},
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
    const client_vm_1_dts_base = b.path(b.fmt("board/{s}/client_vm_1/dts/linux.dts", .{ @tagName(microkit_board) }));
    const client_vm_1_dts_overlays = &[_]LazyPath{
        b.path(b.fmt("board/{s}/client_vm_1/dts/init.dts", .{ @tagName(microkit_board) })),
        b.path(b.fmt("board/{s}/client_vm_1/dts/virtio.dts", .{ @tagName(microkit_board) })),
        b.path(b.fmt("board/{s}/client_vm_1/dts/disable.dts", .{ @tagName(microkit_board) })),
    };
    // Client 1 VM: Pack rootfs
    const client_vm_1_init = libvmm_dep.path("tools/linux/blk/blk_client_init");
    const client_vm_1_rootfs_base = b.path(b.fmt("board/{s}/client_vm_1/rootfs.cpio.gz", .{ @tagName(microkit_board) }));
    const client_vmm_1 = addVmm(
        b,
        "client_vm_1",
        "client_vmm_1.elf",
        client_src_file,
        client_vm_1_rootfs_base,
        &.{},
        &.{client_vm_1_init},
        client_vm_1_dts_base,
        client_vm_1_dts_overlays,
        target,
        optimize
    );
    b.installArtifact(client_vmm_1);

    // Client 2 VMM artifact
    // Client 2 VM: Get DTB
    const client_vm_2_dts_base = b.path(b.fmt("board/{s}/client_vm_2/dts/linux.dts", .{ @tagName(microkit_board) }));
    const client_vm_2_dts_overlays = &[_]LazyPath{
        b.path(b.fmt("board/{s}/client_vm_2/dts/init.dts", .{ @tagName(microkit_board) })),
        b.path(b.fmt("board/{s}/client_vm_2/dts/virtio.dts", .{ @tagName(microkit_board) })),
        b.path(b.fmt("board/{s}/client_vm_2/dts/disable.dts", .{ @tagName(microkit_board) })),
    };
    // Client 2 VM: Pack rootfs
    const client_vm_2_init = libvmm_dep.path("tools/linux/blk/blk_client_init");
    const client_vm_2_rootfs_base = b.path(b.fmt("board/{s}/client_vm_2/rootfs.cpio.gz", .{ @tagName(microkit_board) }));
    const client_vmm_2 = addVmm(
        b,
        "client_vm_2",
        "client_vmm_2.elf",
        client_src_file,
        client_vm_2_rootfs_base,
        &.{},
        &.{client_vm_2_init},
        client_vm_2_dts_base,
        client_vm_2_dts_overlays,
        target,
        optimize
    );
    b.installArtifact(client_vmm_2);

    // UART driver artifact
    const uart_driver = switch (microkit_board) {
        MicrokitBoard.qemu_arm_virt => sddf_dep.artifact("driver_uart_arm.elf"),
        MicrokitBoard.odroidc4 => sddf_dep.artifact("driver_uart_meson.elf"),
    };
    const uart_driver_install = b.addInstallArtifact(uart_driver, .{ .dest_sub_path = "uart_driver.elf" });
    b.getInstallStep().dependOn(&uart_driver_install.step);

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
    const system_description_path = LazyPath{ .cwd_relative = b.fmt("board/{s}/virtio.system", .{ @tagName(microkit_board) }) };
    const final_image_dest = b.getInstallPath(.bin, "./loader.img");
    const microkit_tool_cmd = b.addSystemCommand(&[_][]const u8{
        microkit_tool.getPath(b),
    });
    microkit_tool_cmd.addFileArg(system_description_path);
    microkit_tool_cmd.addArg("--search-path");
    microkit_tool_cmd.addArg(b.getInstallPath(.bin, ""));
    microkit_tool_cmd.addArg("--board");
    microkit_tool_cmd.addArg(@tagName(microkit_board));
    microkit_tool_cmd.addArg("--config");
    microkit_tool_cmd.addArg(@tagName(microkit_config));
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
    if (microkit_board == MicrokitBoard.qemu_arm_virt) {
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
