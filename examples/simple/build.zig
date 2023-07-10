const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    // @ivanv: need to somehow relate sel4cp config with this optimisation level?
    const optimize = b.standardOptimizeOption(.{});

    // @ivanv sort out
    const sdk_path = "/Users/ivanv/ts/sel4cp_dev/release/sel4cp-sdk-1.2.6";
    const board = "qemu_arm_virt_hyp";
    const config = "debug";
    // Since we are relying on Zig to produce the final ELF, it needs to do the
    // linking step as well.
    const sdk_board_dir = sdk_path ++ "/board/" ++ board ++ "/" ++ config;

    const vmmlib = b.addStaticLibrary(.{
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
    vmmlib.addCSourceFiles(&.{
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
    }, &.{
        "-Wall",
        "-Werror",
        "-Wno-unused-function",
        "-mstrict-align",
        "-DBOARD_qemu_arm_virt_hyp",
    });

    vmmlib.addIncludePath(vmm_src_dir);
    vmmlib.addIncludePath(vmm_src_dir ++ "util/");
    vmmlib.addIncludePath(vmm_src_arch_dir);
    vmmlib.addIncludePath(vmm_src_arch_dir ++ "vgic/");
    vmmlib.addIncludePath(sdk_board_dir ++ "/include");

    b.installArtifact(vmmlib);

    const exe = b.addExecutable(.{
        .name = "vmm.elf",
        .target = target,
        .optimize = optimize,
    });

    // Add sel4cp.h to be used by the API wrapper.
    exe.addIncludePath(sdk_board_dir ++ "/include");
    exe.addIncludePath(vmm_src_dir);
    exe.addIncludePath(vmm_src_arch_dir);
    // Add the static library that provides each protection domain's entry
    // point (`main()`), which runs the main handler loop.
    exe.addObjectFile(sdk_board_dir ++ "/lib/libsel4cp.a");
    exe.linkLibrary(vmmlib);
    // Specify the linker script, this is necessary to set the ELF entry point address.
    exe.setLinkerScriptPath(.{ .path = sdk_board_dir ++ "/lib/sel4cp.ld"});

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
       sdk_path ++ "/bin/sel4cp",
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

    // This is setting up a `simulate` command for running the system via QEMU,
    // which we only want to do when we have a board that we can actually simulate.
    // @ivanv we should try get renode working as well
    if (std.mem.eql(u8, board, "qemu_arm_virt_hyp")) {
        const simulate_cmd = b.addSystemCommand(&[_][]const u8{
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
        simulate_cmd.step.dependOn(b.default_step);
        const simulate_step = b.step("simulate", "Simulate the image via QEMU");
        simulate_step.dependOn(&simulate_cmd.step);
    }
}
