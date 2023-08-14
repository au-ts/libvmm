const std = @import("std");

// For this example we hard-code the target to AArch64 and the platform to QEMU ARM virt
// since the main point of this example is to show off using the VMM library in another
// systems programming language.
const example_target = std.zig.CrossTarget{
    .cpu_arch = .aarch64,
    .cpu_model = .{ .explicit = &std.Target.arm.cpu.cortex_a53 },
    .os_tag = .freestanding,
    .abi = .none,
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{ .default_target = example_target });
    const optimize = b.standardOptimizeOption(.{});

    // @ivanv sort out
    const sdk_path = "/home/ivanv/ts/sel4cp/release/sel4cp-sdk-1.2.6";
    const board = "qemu_arm_virt";
    const config = "debug";
    // const sel4cp_build_dir = "build";
    // Since we are relying on Zig to produce the final ELF, it needs to do the
    // linking step as well.
    const sdk_board_dir = sdk_path ++ "/board/" ++ board ++ "/" ++ config;

    const zig_libsel4cp = b.addObject(.{
        .name = "zig_libsel4cp",
        .root_source_file = .{ .path = "src/libsel4cp.c" },
        .target = target,
        .optimize = optimize,
    });
    zig_libsel4cp.addIncludePath(.{ .path = "src/" });
    zig_libsel4cp.addIncludePath(.{ .path = sdk_board_dir ++ "/include" });

    const vmmlib = b.addStaticLibrary(.{
        .name = "vmm",
        .target = target,
        .optimize = optimize,
    });

    vmmlib.addCSourceFiles(&.{
        "../../src/guest.c",
        "../../src/util/util.c",
        "../../src/util/printf.c",
        "../../src/arch/aarch64/vgic/vgic.c",
        "../../src/arch/aarch64/vgic/vgic_v2.c",
        "../../src/arch/aarch64/fault.c",
        "../../src/arch/aarch64/psci.c",
        "../../src/arch/aarch64/smc.c",
        "../../src/arch/aarch64/virq.c",
        "../../src/arch/aarch64/linux.c",
        "../../src/arch/aarch64/vcpu.c",
        "../../src/arch/aarch64/tcb.c",
    }, &.{
        "-Wall",
        "-Werror",
        "-Wno-unused-function",
        "-mstrict-align",
        "-DBOARD_qemu_arm_virt",
        "-nostdlib",
        "-ffreestanding",
        "-mcpu=cortex-a53",
    });

    vmmlib.addIncludePath(.{ .path = "../../src/" });
    vmmlib.addIncludePath(.{ .path = "../../src/util/" });
    vmmlib.addIncludePath(.{ .path = "../../src/arch/aarch64/" });
    vmmlib.addIncludePath(.{ .path = "../../src/arch/aarch64/vgic/" });
    vmmlib.addIncludePath(.{ .path = sdk_board_dir ++ "/include" });

    b.installArtifact(vmmlib);

    const exe = b.addExecutable(.{
        .name = "vmm.elf",
        .root_source_file = .{ .path = "src/vmm.zig" },
        .target = target,
        .optimize = optimize,
    });

    // Add sel4cp.h to be used by the API wrapper.
    exe.addIncludePath(.{ .path = sdk_board_dir ++ "/include" });
    exe.addIncludePath(.{ .path = "../../src/" });
    exe.addIncludePath(.{ .path = "../../src/util/" });
    exe.addIncludePath(.{ .path = "../../src/arch/aarch64/" });
    // Add the static library that provides each protection domain's entry
    // point (`main()`), which runs the main handler loop.
    exe.addObjectFile(.{ .path = sdk_board_dir ++ "/lib/libsel4cp.a" });
    exe.linkLibrary(vmmlib);
    exe.addObject(zig_libsel4cp);
    // exe.linkLibrary(libsel4);
    // Specify the linker script, this is necessary to set the ELF entry point address.
    exe.setLinkerScriptPath(.{ .path = sdk_board_dir ++ "/lib/sel4cp.ld"});

    exe.addIncludePath(.{ .path = "src/" });

    b.installArtifact(exe);

    const system_description_path = "zig_vmm.system";
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
    sel4cp_tool_cmd.step.dependOn(b.getInstallStep());
    // Add the "sel4cp" step, and make it the default step when we execute `zig build`>
    const sel4cp_step = b.step("sel4cp", "Compile and build the final bootable image");
    sel4cp_step.dependOn(&sel4cp_tool_cmd.step);
    b.default_step = sel4cp_step;

    // This is setting up a `qemu` command for running the system via QEMU,
    // which we only want to do when we have a board that we can actually simulate.
    // @ivanv we should try get renode working as well
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
    const simulate_step = b.step("qemu", "Simulate the image using QEMU");
    simulate_step.dependOn(&qemu_cmd.step);

    // const renode_cmd = b.addSystemCommand(&[_][]const u8{
    //     "qemu-system-aarch64",
    //     "-machine",
    //     "virt,virtualization=on,highmem=off,secure=off",
    //     "-cpu",
    //     "cortex-a53",
    //     "-serial",
    //     "mon:stdio",
    //     "-device",
    //     "loader,file=zig-out/bin/loader.img,addr=0x70000000,cpu-num=0",
    //     "-m",
    //     "2G",
    //     "-nographic",
    // });
    // renode_cmd.step.dependOn(b.default_step);
    // const simulate_step = b.step("renode", "Simulate the image using Renode");
    // simulate_step.dependOn(&renode_cmd.step);
}
