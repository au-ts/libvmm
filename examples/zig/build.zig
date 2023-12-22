const std = @import("std");

var general_purpose_allocator = std.heap.GeneralPurposeAllocator(.{}){};
const gpa = general_purpose_allocator.allocator();

fn fmtPrint(comptime fmt: []const u8, args: anytype) []const u8 {
    return std.fmt.allocPrint(gpa, fmt, args) catch "Could not format print!";
}

// For this example we hard-code the target to AArch64 and the platform to QEMU ARM virt
// since the main point of this example is to show off using libvmm in another
// systems programming language.
const example_target = std.zig.CrossTarget{
    .cpu_arch = .aarch64,
    .cpu_model = .{ .explicit = &std.Target.arm.cpu.cortex_a53 },
    .os_tag = .freestanding,
    .abi = .none,
};

const ConfigOptions = enum {
    debug,
    release,
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{ .default_target = example_target });
    const optimize = b.standardOptimizeOption(.{});

    // Getting the path to the Microkit SDK before doing anything else
    const microkit_sdk_arg = b.option([]const u8, "sdk", "Path to Microkit SDK");
    if (microkit_sdk_arg == null) {
        std.log.err("Missing -Dsdk=/path/to/sdk argument being passed\n", .{});
        std.os.exit(1);
    }
    const microkit_sdk = microkit_sdk_arg.?;

    // Hard-code the board to QEMU ARM virt since it's the only one the example intends to support
    const microkit_board = "qemu_arm_virt";
    const microkit_config_option = b.option(ConfigOptions, "config", "Microkit config to build for") orelse ConfigOptions.debug;
    const microkit_config = @tagName(microkit_config_option);
    // Since we are relying on Zig to produce the final ELF, it needs to do the
    // linking step as well.
    const microkit_board_dir = fmtPrint("{s}/board/{s}/{s}", .{ microkit_sdk, microkit_board, microkit_config });
    const microkit_tool = fmtPrint("{s}/bin/microkit", .{ microkit_sdk });
    const libmicrokit = fmtPrint("{s}/lib/libmicrokit.a", .{ microkit_board_dir });
    const libmicrokit_linker_script = fmtPrint("{s}/lib/microkit.ld", .{ microkit_board_dir });
    const sdk_board_include_dir = fmtPrint("{s}/include", .{ microkit_board_dir });

    const zig_libmicrokit = b.addObject(.{
        .name = "zig_libmicrokit",
        .root_source_file = .{ .path = "src/libmicrokit.c" },
        .target = target,
        .optimize = optimize,
    });
    zig_libmicrokit.addIncludePath(.{ .path = "src/" });
    zig_libmicrokit.addIncludePath(.{ .path = sdk_board_include_dir });

    const libvmm = b.addStaticLibrary(.{
        .name = "vmm",
        .target = target,
        .optimize = optimize,
    });

    const libvmm_path = "../..";
    const libvmm_src = libvmm_path ++ "/src/";
    // Right now we only support AArch64 so this is a safe assumption.
    const libvmm_src_arch = libvmm_src ++ "arch/aarch64/";
    libvmm.addCSourceFiles(.{
        .files = &.{
            libvmm_src ++ "guest.c",
            libvmm_src ++ "util/util.c",
            libvmm_src ++ "util/printf.c",
            libvmm_src_arch ++ "vgic/vgic.c",
            libvmm_src_arch ++ "vgic/vgic_v2.c",
            libvmm_src_arch ++ "fault.c",
            libvmm_src_arch ++ "psci.c",
            libvmm_src_arch ++ "smc.c",
            libvmm_src_arch ++ "virq.c",
            libvmm_src_arch ++ "linux.c",
            libvmm_src_arch ++ "tcb.c",
            libvmm_src_arch ++ "vcpu.c",
        },
        .flags = &.{
            "-Wall",
            "-Werror",
            "-Wno-unused-function",
            "-mstrict-align",
            "-DBOARD_qemu_arm_virt", // @ivanv: should not be necessary
            "-fno-sanitize=undefined", // @ivanv: ideally we wouldn't have to turn off UBSAN
        }
    });

    libvmm.addIncludePath(.{ .path = libvmm_src });
    libvmm.addIncludePath(.{ .path = libvmm_src ++ "util/" });
    libvmm.addIncludePath(.{ .path = libvmm_src_arch });
    libvmm.addIncludePath(.{ .path = libvmm_src_arch ++ "vgic/" });
    libvmm.addIncludePath(.{ .path = sdk_board_include_dir });

    b.installArtifact(libvmm);

    const exe = b.addExecutable(.{
        .name = "vmm.elf",
        .root_source_file = .{ .path = "src/vmm.zig" },
        .target = target,
        .optimize = optimize,
    });
    // Microkit expects and requires the symbol table to exist in the ELF,
    // this means that even when building for release mode, we want to tell
    // Zig not to strip symbols from the binary.
    exe.strip = false;

    // For actually compiling the DTS into a DTB
    const dts_path = "images/linux.dts";
    const dtc_cmd = b.addSystemCommand(&[_][]const u8{
        "dtc", "-q", "-I", "dts", "-O", "dtb", dts_path
    });
    const dtb = dtc_cmd.captureStdOut();
    // When we embed these artifacts into our VMM code, we use @embedFile provided by
    // the Zig compiler. However, we can't just include any path outside of the 'src/'
    // directory and so we add each file as a "module".
    exe.addAnonymousModule("linux.dtb", .{ .source_file = dtb });
    exe.addAnonymousModule("linux", .{ .source_file = .{ .path = "images/linux" } });
    exe.addAnonymousModule("rootfs.cpio.gz", .{ .source_file = .{ .path = "images/rootfs.cpio.gz" } });

    // Add microkit.h to be used by the API wrapper.
    exe.addIncludePath(.{ .path = sdk_board_include_dir });
    exe.addIncludePath(.{ .path = "../../src/" });
    exe.addIncludePath(.{ .path = "../../src/util/" });
    exe.addIncludePath(.{ .path = "../../src/arch/aarch64/" });
    // Add the static library that provides each protection domain's entry
    // point (`main()`), which runs the main handler loop.
    exe.addObjectFile(.{ .path = libmicrokit });
    exe.linkLibrary(libvmm);
    exe.addObject(zig_libmicrokit);
    // Specify the linker script, this is necessary to set the ELF entry point address.
    exe.setLinkerScriptPath(.{ .path = libmicrokit_linker_script });

    exe.addIncludePath(.{ .path = "src/" });

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

    // This is setting up a `qemu` command for running the system via QEMU.
    const loader_arg = fmtPrint("loader,file={s},addr=0x70000000,cpu-num=0", .{ final_image_dest });
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
