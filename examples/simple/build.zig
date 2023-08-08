const std = @import("std");

var general_purpose_allocator = std.heap.GeneralPurposeAllocator(.{}){};
const gpa = general_purpose_allocator.allocator();

fn concatStr(strings: []const []const u8) []const u8 {
    return std.mem.concat(gpa, u8, strings) catch "";
}

const configOptions = enum {
    debug,
    release,
    benchmark
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    // @ivanv: need to somehow relate sel4cp config with this optimisation level?
    const optimize = b.standardOptimizeOption(.{});

    // Getting the path to the seL4CP SDK before doing anything else
    const sel4cp_sdk = b.option([]const u8, "sdk", "Path to seL4CP SDK") orelse null;
    if (sel4cp_sdk == null) {
        std.log.err("Missing -Dsdk=/path/to/sdk argument being passed\n", .{});
        std.os.exit(1);
    }

    const sel4cp_config_option = b.option(configOptions, "config", "seL4CP config to build for")
                                 orelse configOptions.debug;
    const sel4cp_config_str = @tagName(sel4cp_config_option);

    // Get the seL4CP SDK board we want to target
    const sel4cp_board = b.option([]const u8, "board", "seL4CP board to target")
                                 orelse null;
    if (sel4cp_board == null) {
        std.log.err("Missing -Dboard=<BOARD> argument being passed\n", .{});
        std.os.exit(1);
    }

    // @ivanv sort out
    const board = "qemu_arm_virt_hyp";
    const config = "debug";

    // Since we are relying on Zig to produce the final ELF, it needs to do the
    // linking step as well.
    const sdk_board_dir = concatStr(&[_][]const u8{ sel4cp_sdk.?, "/board/", sel4cp_board.?, "/", sel4cp_config_str });
    const sdk_tool = concatStr(&[_][]const u8{ sel4cp_sdk.?, "/bin/sel4cp" });
    const libsel4cp = concatStr(&[_][]const u8{ sdk_board_dir, "/lib/libsel4cp.a" });
    const libsel4cp_linker_script = concatStr(&[_][]const u8{ sdk_board_dir, "/lib/sel4cp.ld" });
    const sdk_board_include_dir = concatStr(&[_][]const u8{ sdk_board_dir, "/include" });

    const vmm_lib = b.addStaticLibrary(.{
        .name = "vmm",
        .target = target,
        .optimize = optimize,
    });

    // The paths are required to have quotes around them
    const linux_image_path = "\"board/" ++ board ++ "/linux\"";
    // @ivanv: come back to
    const dtb_image_path = "\"build/linux.dtb\"";
    const initrd_image_path = "\"board/" ++ board ++ "/rootfs.cpio.gz\"";

    const vmm_src_dir = "../../src/";
    // @ivanv: obviously for other archs we want a different dir
    const vmm_src_arch_dir = vmm_src_dir ++ "arch/aarch64/";
    vmm_lib.addCSourceFiles(&.{
        vmm_src_dir ++ "guest.c",
        vmm_src_dir ++ "util/util.c",
        vmm_src_dir ++ "util/printf.c",
        vmm_src_arch_dir ++ "vgic/vgic.c",
        vmm_src_arch_dir ++ "vgic/vgic_v2.c",
        vmm_src_arch_dir ++ "fault.c",
        vmm_src_arch_dir ++ "psci.c",
        vmm_src_arch_dir ++ "smc.c",
        vmm_src_arch_dir ++ "virq.c",
        vmm_src_arch_dir ++ "linux.c",
        vmm_src_arch_dir ++ "tcb.c",
        vmm_src_arch_dir ++ "vcpu.c",
    }, &.{
        "-Wall",
        "-Werror",
        "-Wno-unused-function",
        "-mstrict-align",
        "-DBOARD_qemu_arm_virt_hyp",
    });

    vmm_lib.addIncludePath(.{ .path = vmm_src_dir });
    vmm_lib.addIncludePath(.{ .path = vmm_src_dir ++ "util/" });
    vmm_lib.addIncludePath(.{ .path = vmm_src_arch_dir });
    vmm_lib.addIncludePath(.{ .path = vmm_src_arch_dir ++ "vgic/" });
    vmm_lib.addIncludePath(.{ .path = sdk_board_include_dir });

    b.installArtifact(vmm_lib);

    const exe = b.addExecutable(.{
        .name = "vmm.elf",
        .target = target,
        .optimize = optimize,
    });

    // Add sel4cp.h to be used by the API wrapper.
    exe.addIncludePath(.{ .path = sdk_board_include_dir });
    exe.addIncludePath(.{ .path = vmm_src_dir });
    exe.addIncludePath(.{ .path = vmm_src_arch_dir });
    // Add the static library that provides each protection domain's entry
    // point (`main()`), which runs the main handler loop.
    exe.addObjectFile(.{ .path = libsel4cp });
    exe.linkLibrary(vmm_lib);
    // Specify the linker script, this is necessary to set the ELF entry point address.
    exe.setLinkerScriptPath(.{ .path = libsel4cp_linker_script });

    exe.addCSourceFiles(&.{"vmm.c"}, &.{
        "-Wall",
        "-Werror",
        "-Wno-unused-function",
        "-mstrict-align",
        "-DBOARD_qemu_arm_virt_hyp",
    });

    const guest_images = b.addObject(.{
        .name = "guest_images",
        .target = target,
        .optimize = optimize,
    });

    guest_images.addCSourceFiles(&.{"package_guest_images.S"}, &.{
        "-DGUEST_KERNEL_IMAGE_PATH=" ++ linux_image_path,
        "-DGUEST_DTB_IMAGE_PATH=" ++ dtb_image_path,
        "-DGUEST_INITRD_IMAGE_PATH=" ++ initrd_image_path,
        "-x",
        "assembler-with-cpp",
    });

    exe.addObject(guest_images);
    b.installArtifact(exe);

    const system_description_path = "board/" ++ board ++ "/simple.system";
    const sel4cp_tool_cmd = b.addSystemCommand(&[_][]const u8{
       sdk_tool,
       system_description_path,
       "--search-path",
       "zig-out/bin",
       "--board",
       board,
       "--config",
       config,
       "-o",
       "zig-out/bin/loader.img",
       "-r",
       "zig-out/report.txt",
    });
    // Running the seL4CP tool depends on 
    sel4cp_tool_cmd.step.dependOn(b.getInstallStep());
    // Add the "sel4cp" step, and make it the default step when we execute `zig build`>
    const sel4cp_step = b.step("sel4cp", "Compile and build the final bootable image");
    sel4cp_step.dependOn(&sel4cp_tool_cmd.step);
    b.default_step = sel4cp_step;

    // This is setting up a `qemu` command for running the system via QEMU,
    // which we only want to do when we have a board that we can actually simulate.
    if (std.mem.eql(u8, board, "qemu_arm_virt_hyp")) {
        const qemu_cmd = b.addSystemCommand(&[_][]const u8{
            "qemu-system-aarch64",
            "-machine",
            "virt,virtualization=on,highmem=off,secure=off",
            "-cpu",
            "cortex-a53",
            "-serial",
            "mon:stdio",
            "-device",
            "loader,file=zig-out/bin/loader.img,addr=0x70000000,cpu-num=0",
            "-m",
            "2G",
            "-nographic",
        });
        qemu_cmd.step.dependOn(b.default_step);
        const simulate_step = b.step("qemu", "Simulate the image via QEMU");
        simulate_step.dependOn(&qemu_cmd.step);
    }
}
