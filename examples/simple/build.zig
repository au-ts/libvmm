const std = @import("std");

var general_purpose_allocator = std.heap.GeneralPurposeAllocator(.{}){};
const gpa = general_purpose_allocator.allocator();

const build_dir = "zig-out";
const bin_dir = "zig-out/bin";

fn concatStr(strings: []const []const u8) []const u8 {
    return std.mem.concat(gpa, u8, strings) catch "";
}

const CorePlatformBoard = enum {
    qemu_arm_virt,
    odroidc4
};

const Target = struct {
    board: CorePlatformBoard,
    zig_target: std.zig.CrossTarget,
};

const targets = [_]Target {
    .{
        .board = CorePlatformBoard.qemu_arm_virt,
        .zig_target = std.zig.CrossTarget{
            .cpu_arch = .aarch64,
            .cpu_model = .{ .explicit = &std.Target.arm.cpu.cortex_a53 },
            .os_tag = .freestanding,
            .abi = .none,
        },
    },
    .{
        .board = CorePlatformBoard.odroidc4,
        .zig_target = std.zig.CrossTarget{
            .cpu_arch = .aarch64,
            .cpu_model = .{ .explicit = &std.Target.arm.cpu.cortex_a55 },
            .os_tag = .freestanding,
            .abi = .none,
        },
    }
};

fn findTarget(board: CorePlatformBoard) std.zig.CrossTarget {
    for (targets) |target| {
        if (board == target.board) {
            return target.zig_target;
        }
    }

    std.log.err("Board '{}' is not supported\n", .{ board });
    std.os.exit(1);
}

const ConfigOptions = enum {
    debug,
    release,
    benchmark
};

pub fn build(b: *std.Build) void {
    // @ivanv: need to somehow relate sel4cp config with this optimisation level?
    const optimize = b.standardOptimizeOption(.{});

    // Getting the path to the seL4CP SDK before doing anything else
    const sel4cp_sdk = b.option([]const u8, "sdk", "Path to seL4CP SDK") orelse null;
    if (sel4cp_sdk == null) {
        std.log.err("Missing -Dsdk=/path/to/sdk argument being passed\n", .{});
        std.os.exit(1);
    }

    const sel4cp_config_option = b.option(ConfigOptions, "config", "seL4CP config to build for")
                                 orelse ConfigOptions.debug;
    const sel4cp_config = @tagName(sel4cp_config_option);

    // Get the seL4CP SDK board we want to target
    const sel4cp_board_option = b.option(CorePlatformBoard, "board", "seL4CP board to target")
                                  orelse null;
    if (sel4cp_board_option == null) {
        std.log.err("Missing -Dboard=<BOARD> argument being passed\n", .{});
        std.os.exit(1);
    }

    const target = findTarget(sel4cp_board_option.?);
    const sel4cp_board = @tagName(sel4cp_board_option.?);

    // Since we are relying on Zig to produce the final ELF, it needs to do the
    // linking step as well.
    const sdk_board_dir = concatStr(&[_][]const u8{ sel4cp_sdk.?, "/board/", sel4cp_board, "/", sel4cp_config });
    const sdk_tool = concatStr(&[_][]const u8{ sel4cp_sdk.?, "/bin/sel4cp" });
    const libsel4cp = concatStr(&[_][]const u8{ sdk_board_dir, "/lib/libsel4cp.a" });
    const libsel4cp_linker_script = concatStr(&[_][]const u8{ sdk_board_dir, "/lib/sel4cp.ld" });
    const sdk_board_include_dir = concatStr(&[_][]const u8{ sdk_board_dir, "/include" });

    const libvmm = b.addStaticLibrary(.{
        .name = "vmm",
        .target = target,
        .optimize = optimize,
    });

    const libvmm_src = "../../src/";
    // Right now we only support AArch64 so this is a safe assumption.
    const libvmm_src_arch = libvmm_src ++ "arch/aarch64/";
    libvmm.addCSourceFiles(&.{
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
    }, &.{
        "-Wall",
        "-Werror",
        "-Wno-unused-function",
        "-mstrict-align",
        "-DBOARD_qemu_arm_virt",
    });

    libvmm.addIncludePath(.{ .path = libvmm_src });
    libvmm.addIncludePath(.{ .path = libvmm_src ++ "util/" });
    libvmm.addIncludePath(.{ .path = libvmm_src_arch });
    libvmm.addIncludePath(.{ .path = libvmm_src_arch ++ "vgic/" });
    libvmm.addIncludePath(.{ .path = sdk_board_include_dir });

    b.installArtifact(libvmm);

    const exe = b.addExecutable(.{
        .name = "vmm.elf",
        .target = target,
        .optimize = optimize,
    });

    // For actually compiling the DTS into a DTB
    const dts_path = concatStr(&[_][]const u8{ "board/", sel4cp_board, "/linux.dts" });
    // @ivanv: find a better place, need to figure out zig build scripts more
    const dtb_image_path = "linux.dtb";
    const dtc_command = b.addSystemCommand(&[_][]const u8{
        "dtc", "-I", "dts", "-O", "dtb", dts_path, "-o", dtb_image_path
    });
    exe.step.dependOn(&dtc_command.step);

    const linux_image_path = concatStr(&[_][]const u8{ "board/", sel4cp_board, "/linux" });
    const initrd_image_path = concatStr(&[_][]const u8{ "board/", sel4cp_board, "/rootfs.cpio.gz" });

    // Add sel4cp.h to be used by the API wrapper.
    exe.addIncludePath(.{ .path = sdk_board_include_dir });
    // The VMM code will include 
    exe.addIncludePath(.{ .path = libvmm_src });
    exe.addIncludePath(.{ .path = libvmm_src_arch });
    // Add the static library that provides each protection domain's entry
    // point (`main()`), which runs the main handler loop.
    exe.addObjectFile(.{ .path = libsel4cp });
    exe.linkLibrary(libvmm);
    // Specify the linker script, this is necessary to set the ELF entry point address.
    exe.setLinkerScriptPath(.{ .path = libsel4cp_linker_script });

    exe.addCSourceFiles(&.{"vmm.c"}, &.{
        "-Wall",
        "-Werror",
        "-Wno-unused-function",
        "-mstrict-align",
        "-DBOARD_qemu_arm_virt",
    });

    const guest_images = b.addObject(.{
        .name = "guest_images",
        .target = target,
        .optimize = optimize,
    });

    const kernel_image_arg = concatStr(&[_][]const u8{ "-DGUEST_KERNEL_IMAGE_PATH=", "\"", linux_image_path, "\"" });
    const dtb_image_arg = concatStr(&[_][]const u8{ "-DGUEST_DTB_IMAGE_PATH=", "\"", dtb_image_path, "\"" });
    const initrd_image_arg = concatStr(&[_][]const u8{ "-DGUEST_INITRD_IMAGE_PATH=", "\"", initrd_image_path, "\"" });
    guest_images.addCSourceFiles(&.{"package_guest_images.S"}, &.{
        kernel_image_arg,
        dtb_image_arg,
        initrd_image_arg,
        "-x",
        "assembler-with-cpp",
    });

    exe.addObject(guest_images);
    b.installArtifact(exe);

    const system_description_path = concatStr(&[_][]const u8{ "board/", sel4cp_board, "/simple.system" });
    const final_image_path = bin_dir ++ "/loader.img";
    const sel4cp_tool_cmd = b.addSystemCommand(&[_][]const u8{
       sdk_tool,
       system_description_path,
       "--search-path",
       bin_dir,
       "--board",
       sel4cp_board,
       "--config",
       sel4cp_config,
       "-o",
       final_image_path,
       "-r",
       build_dir ++ "/report.txt",
    });
    // Running the seL4CP tool depends on 
    sel4cp_tool_cmd.step.dependOn(b.getInstallStep());
    // Add the "sel4cp" step, and make it the default step when we execute `zig build`>
    const sel4cp_step = b.step("sel4cp", "Compile and build the final bootable image");
    sel4cp_step.dependOn(&sel4cp_tool_cmd.step);
    b.default_step = sel4cp_step;

    // This is setting up a `qemu` command for running the system via QEMU,
    // which we only want to do when we have a board that we can actually simulate.
    if (std.mem.eql(u8, sel4cp_board, "qemu_arm_virt")) {
        const qemu_cmd = b.addSystemCommand(&[_][]const u8{
            "qemu-system-aarch64",
            "-machine",
            "virt,virtualization=on,highmem=off,secure=off",
            "-cpu",
            "cortex-a53",
            "-serial",
            "mon:stdio",
            "-device",
            "loader,file=" ++ final_image_path ++ ",addr=0x70000000,cpu-num=0",
            "-m",
            "2G",
            "-nographic",
        });
        qemu_cmd.step.dependOn(b.default_step);
        const simulate_step = b.step("qemu", "Simulate the image via QEMU");
        simulate_step.dependOn(&qemu_cmd.step);
    }
}
