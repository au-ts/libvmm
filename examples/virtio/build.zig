const std = @import("std");
const LazyPath = std.Build.LazyPath;

const MicrokitBoard = enum {
    qemu_arm_virt,
    odroidc4
};

const Target = struct {
    board: MicrokitBoard,
    zig_target: std.zig.CrossTarget,
};

const targets = [_]Target {
    .{
        .board = MicrokitBoard.qemu_arm_virt,
        .zig_target = std.zig.CrossTarget{
            .cpu_arch = .aarch64,
            .cpu_model = .{ .explicit = &std.Target.arm.cpu.cortex_a53 },
            .os_tag = .freestanding,
            .abi = .none,
        },
    },
    .{
        .board = MicrokitBoard.odroidc4,
        .zig_target = std.zig.CrossTarget{
            .cpu_arch = .aarch64,
            .cpu_model = .{ .explicit = &std.Target.arm.cpu.cortex_a55 },
            .os_tag = .freestanding,
            .abi = .none,
        },
    }
};

fn findTarget(board: MicrokitBoard) std.zig.CrossTarget {
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

fn dtscat(b: *std.Build, dtscat_path: LazyPath, base: []const u8, overlays: []const []const u8) LazyPath {
    const dtc_cmd = b.addSystemCommand(&[_][]const u8{
        "sh"
    });
    dtc_cmd.addFileArg(dtscat_path);
    dtc_cmd.addFileArg(b.path(base));
    for (overlays) |overlay| {
        dtc_cmd.addFileArg(b.path(overlay));
    }

    return dtc_cmd.captureStdOut();
}

pub fn build(b: *std.Build) void {
    const optimize = b.standardOptimizeOption(.{});

    // Getting the path to the Microkit SDK before doing anything else
    const microkit_sdk_arg = b.option([]const u8, "sdk", "Path to Microkit SDK");
    if (microkit_sdk_arg == null) {
        std.log.err("Missing -Dsdk=/path/to/sdk argument being passed\n", .{});
        std.posix.exit(1);
    }
    const microkit_sdk = microkit_sdk_arg.?;

    const microkit_config_option = b.option(ConfigOptions, "config", "Microkit config to build for") orelse ConfigOptions.debug;
    const microkit_config = @tagName(microkit_config_option);

    // Get the Microkit SDK board we want to target
    const microkit_board_option = b.option(MicrokitBoard, "board", "Microkit board to target");

    if (microkit_board_option == null) {
        std.log.err("Missing -Dboard=<BOARD> argument being passed\n", .{});
        std.posix.exit(1);
    }
    const target = b.resolveTargetQuery(findTarget(microkit_board_option.?));
    const microkit_board = @tagName(microkit_board_option.?);

    // Since we are relying on Zig to produce the final ELF, it needs to do the
    // linking step as well.
    const microkit_board_dir = b.fmt("{s}/board/{s}/{s}", .{ microkit_sdk, microkit_board, microkit_config });
    const microkit_tool = b.fmt("{s}/bin/microkit", .{ microkit_sdk });
    const libmicrokit = .{ .path = b.fmt("{s}/lib/libmicrokit.a", .{ microkit_board_dir }) };
    const libmicrokit_linker_script = .{ .path = b.fmt("{s}/lib/microkit.ld", .{ microkit_board_dir }) };
    const libmicrokit_include = .{ .path = b.fmt("{s}/include", .{ microkit_board_dir }) };

    const libvmm_dep = b.dependency("libvmm", .{
        .target = target,
        .optimize = optimize,
        .sdk = microkit_sdk,
        .config = @as([]const u8, microkit_config),
        .board = @as([]const u8, microkit_board),
    });

    // This will become libvmm.a that we link with each VMM PD
    const libvmm = libvmm_dep.artifact("vmm");

    const sddf_dep = b.dependency("sddf", .{
        .target = target,
        .optimize = optimize,
        .sdk = microkit_sdk,
        .config = @as([]const u8, microkit_config),
        .board = @as([]const u8, microkit_board),
    });

    // const client_vmm_1 = b.addExecutable(.{
    //     .name = "client_vmm_1.elf",
    //     .target = target,
    //     .optimize = optimize,
    //     // Microkit expects and requires the symbol table to exist in the ELF,
    //     // this means that even when building for release mode, we want to tell
    //     // Zig not to strip symbols from the binary.
    //     .strip = false,
    // });

    const blk_driver_vmm = b.addExecutable(.{
        .name = "blk_driver_vmm.elf",
        .target = target,
        .optimize = optimize,
        // Microkit expects and requires the symbol table to exist in the ELF,
        // this means that even when building for release mode, we want to tell
        // Zig not to strip symbols from the binary.
        .strip = false,
    });

    const dtscat_path = libvmm_dep.path("tools/dtscat");

    const blk_driver_vm_dts_base = b.fmt("board/{s}/blk_driver_vm/dts/linux.dts", .{ microkit_board });
    const blk_driver_vm_dts_overlays = &[_][]const u8{
        b.fmt("board/{s}/blk_driver_vm/dts/init.dts", .{ microkit_board }),
        b.fmt("board/{s}/blk_driver_vm/dts/io.dts", .{ microkit_board }),
        b.fmt("board/{s}/blk_driver_vm/dts/disable.dts", .{ microkit_board }),
    };
    const blk_driver_vm_dtb = dtscat(b, dtscat_path, blk_driver_vm_dts_base, blk_driver_vm_dts_overlays);

    // Add microkit.h to be used by the API wrapper.
    blk_driver_vmm.addIncludePath(libvmm_dep.path("src"));
    blk_driver_vmm.addIncludePath(libvmm_dep.path("src/util"));
    blk_driver_vmm.addIncludePath(libvmm_dep.path("src/virtio"));
    blk_driver_vmm.addIncludePath(libvmm_dep.path("src/arch/aarch64"));
    blk_driver_vmm.addIncludePath(sddf_dep.path("include"));
    // Add the static library that provides each protection domain's entry
    // point (`main()`), which runs the main handler loop.
    blk_driver_vmm.linkLibrary(libvmm);
    // Do Microkit specific setup
    blk_driver_vmm.setLinkerScriptPath(libmicrokit_linker_script);
    blk_driver_vmm.addIncludePath(libmicrokit_include);
    blk_driver_vmm.addObjectFile(libmicrokit);

    blk_driver_vmm.addCSourceFiles(.{
        .files = &.{"blk_driver_vmm.c"},
        .flags = &.{
            "-Wall",
            "-Werror",
            "-Wno-unused-function",
            "-mstrict-align",
            b.fmt("-DBOARD_{s}", .{ microkit_board })
        }
    });

    const blk_driver_vm_images = b.addObject(.{
        .name = "blk_driver_vm_images",
        .target = target,
        .optimize = optimize,
    });
    // We need to produce the DTB from the DTS before doing anything to produce guest_images
    const blk_driver_vm_dtb_name = "blk_driver_vm.dtb";
    blk_driver_vm_images.step.dependOn(&b.addInstallFileWithDir(blk_driver_vm_dtb, .prefix, blk_driver_vm_dtb_name).step);

    const linux_image_path = b.fmt("board/{s}/blk_driver_vm/linux", .{ microkit_board });
    const kernel_image_arg = b.fmt("-DGUEST_KERNEL_IMAGE_PATH=\"{s}\"", .{ linux_image_path });

    const initrd_image_path = b.fmt("board/{s}/blk_driver_vm/rootfs.cpio.gz", .{ microkit_board });
    const initrd_image_arg = b.fmt("-DGUEST_INITRD_IMAGE_PATH=\"{s}\"", .{ initrd_image_path });
    const dtb_image_arg = b.fmt("-DGUEST_DTB_IMAGE_PATH=\"{s}\"", .{ b.getInstallPath(.prefix, blk_driver_vm_dtb_name) });
    blk_driver_vm_images.addCSourceFile(.{
        .file = libvmm_dep.path("tools/package_guest_images.S"),
        .flags = &.{
            kernel_image_arg,
            dtb_image_arg,
            initrd_image_arg,
            "-x",
            "assembler-with-cpp",
        }
    });

    blk_driver_vmm.addObject(blk_driver_vm_images);
    b.installArtifact(blk_driver_vmm);

    const uart_driver = sddf_dep.artifact("driver_uart_arm.elf");
    b.installArtifact(uart_driver);

    const serial_virt_tx = sddf_dep.artifact("serial_component_virt_tx.elf");
    b.installArtifact(serial_virt_tx);

    const serial_virt_rx = sddf_dep.artifact("serial_component_virt_rx.elf");
    b.installArtifact(serial_virt_rx);

    const system_description_path = b.fmt("board/{s}/virtio.system", .{ microkit_board });
    const final_image_dest = b.getInstallPath(.bin, "./loader.img");
    const microkit_tool_cmd = b.addSystemCommand(&[_][]const u8{
       microkit_tool,
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
    // Add the "microkit" step, and make it the default step when we execute `zig build`>
    const microkit_step = b.step("microkit", "Compile and build the final bootable image");
    microkit_step.dependOn(&microkit_tool_cmd.step);
    b.default_step = microkit_step;

    // This is setting up a `qemu` command for running the system using QEMU,
    // which we only want to do when we have a board that we can actually simulate.
    const loader_arg = b.fmt("loader,file={s},addr=0x70000000,cpu-num=0", .{ final_image_dest });
    if (std.mem.eql(u8, microkit_board, "qemu_arm_virt")) {
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
}
