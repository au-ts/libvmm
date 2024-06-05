const std = @import("std");

const ConfigOptions = enum {
    debug,
    release,
};

pub fn build(b: *std.Build) void {
    // For this example we hard-code the target to AArch64 and the platform to QEMU ARM virt
    // since the main point of this example is to show off using libvmm in another
    // systems programming language.
    const target = b.resolveTargetQuery(.{
        .cpu_arch = .aarch64,
        .cpu_model = .{ .explicit = &std.Target.arm.cpu.cortex_a53 },
        .os_tag = .freestanding,
        .abi = .none,
    });
    const optimize = b.standardOptimizeOption(.{});

    // Getting the path to the Microkit SDK before doing anything else
    const microkit_sdk_arg = b.option([]const u8, "sdk", "Path to Microkit SDK");
    if (microkit_sdk_arg == null) {
        std.log.err("Missing -Dsdk=/path/to/sdk argument being passed\n", .{});
        std.posix.exit(1);
    }
    const microkit_sdk = microkit_sdk_arg.?;

    // Hard-code the board to QEMU ARM virt since it's the only one the example intends to support
    const microkit_board = "qemu_arm_virt";
    const microkit_config_option = b.option(ConfigOptions, "config", "Microkit config to build for") orelse ConfigOptions.debug;
    const microkit_config = @tagName(microkit_config_option);
    // Since we are relying on Zig to produce the final ELF, it needs to do the
    // linking step as well.
    const microkit_board_dir = b.fmt("{s}/board/{s}/{s}", .{ microkit_sdk, microkit_board, microkit_config });
    const microkit_tool = b.fmt("{s}/bin/microkit", .{ microkit_sdk });
    const libmicrokit = b.fmt("{s}/lib/libmicrokit.a", .{ microkit_board_dir });
    const libmicrokit_linker_script = b.fmt("{s}/lib/microkit.ld", .{ microkit_board_dir });
    const sdk_board_include_dir = b.fmt("{s}/include", .{ microkit_board_dir });

    const zig_libmicrokit = b.addObject(.{
        .name = "zig_libmicrokit",
        .target = target,
        .optimize = optimize,
    });
    zig_libmicrokit.addCSourceFile(.{ .file = b.path("src/libmicrokit.c"), .flags = &.{} });
    zig_libmicrokit.addIncludePath(b.path("src/"));
    zig_libmicrokit.addIncludePath(.{ .cwd_relative = sdk_board_include_dir });

    const libvmm_dep = b.dependency("libvmm", .{
        .target = target,
        .optimize = optimize,
        .sdk = microkit_sdk,
        .config = @as([]const u8, microkit_config),
        .board = @as([]const u8, microkit_board),
    });
    const libvmm = libvmm_dep.artifact("vmm");

    const exe = b.addExecutable(.{
        .name = "vmm.elf",
        .root_source_file = b.path("src/vmm.zig"),
        .target = target,
        .optimize = optimize,
        // Microkit expects and requires the symbol table to exist in the ELF,
        // this means that even when building for release mode, we want to tell
        // Zig not to strip symbols from the binary.
        .strip = false,
    });

    // For actually compiling the DTS into a DTB
    const dtc_cmd = b.addSystemCommand(&[_][]const u8{
        "dtc", "-q", "-I", "dts", "-O", "dtb"
    });
    dtc_cmd.addFileArg(b.path("images/linux.dts"));
    const dtb = dtc_cmd.captureStdOut();
    // When we embed these artifacts into our VMM code, we use @embedFile provided by
    // the Zig compiler. However, we can't just include any path outside of the 'src/'
    // directory and so we add each file as a "module".
    exe.root_module.addAnonymousImport("dtb", .{ .root_source_file = dtb });
    exe.root_module.addAnonymousImport("linux", .{ .root_source_file = b.path("images/linux") });
    exe.root_module.addAnonymousImport("rootfs", .{ .root_source_file = b.path("images/rootfs.cpio.gz") });

    // Add microkit.h to be used by the API wrapper.
    exe.addIncludePath(.{ .cwd_relative = sdk_board_include_dir });
    exe.addIncludePath(libvmm_dep.path("src"));
    // @ivanv: shouldn't need to do this! fix our includes
    exe.addIncludePath(libvmm_dep.path("src/arch/aarch64"));
    // Add the static library that provides each protection domain's entry
    // point (`main()`), which runs the main handler loop.
    exe.addObjectFile(.{ .cwd_relative = libmicrokit });
    exe.linkLibrary(libvmm);
    exe.addObject(zig_libmicrokit);
    // Specify the linker script, this is necessary to set the ELF entry point address.
    exe.setLinkerScriptPath(.{ .cwd_relative = libmicrokit_linker_script });

    exe.addIncludePath(b.path("src/"));

    b.installArtifact(exe);

    const system_description_path = "zig_vmm.system";
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
