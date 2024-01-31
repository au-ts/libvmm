# The following is a proof-of-concept for using the Meson build system.
# Needs to be cleaned up but is a good baseline.
#
# Benefits over Make:
#  - less per-project duplication
#  - better dependency tracking
#  - uses Ninja (faster)
# Down sides: (tentative)
#  - less widely available; heavyweight
#
# Setup:
#   meson setup --cross-file targets/aarch64.ini \
#       -Dmicrokit=/path/to/microkit-sdk \
#       -Dboard=qemu_arm_virt build
# Build:
#   ninja -C build
# TODO:
#  - Optionally include virtio libraries
#  - Streamline example projects -- maybe could predefine commands for custom targets
#  - Allow building of userlevel libraries for virtIO examples (this will be difficult)
#  - vgic_v3.c

project('libvmm', 'c')
c = meson.get_compiler('c')

libvmm_src = [
    'src/guest.c',

    'src/util/printf.c',
    'src/util/util.c',

    # When we eventually support other platforms, these can be moved
    'src/arch/aarch64/fault.c',
    'src/arch/aarch64/linux.c',
    'src/arch/aarch64/psci.c',
    'src/arch/aarch64/smc.c',
    'src/arch/aarch64/tcb.c',
    'src/arch/aarch64/vcpu.c',
    'src/arch/aarch64/virq.c',
    'src/arch/aarch64/vgic/vgic_v2.c',
    'src/arch/aarch64/vgic/vgic.c',
    # TODO
    # 'src/arch/aarch64/vgic/vgic_v3.c',
    # 'src/virtio/console.c',
    # 'src/virtio/mmio.c',
]

board = get_option('board')
board_dir = get_option('microkit') / 'board' / board / get_option('buildtype')
buildtype = get_option('buildtype')

libvmm_defines = [
    '-DBOARD_' + board,
    '-DCONFIG_' + buildtype,
]

libvmm_includes = [
    include_directories('src/'),
    include_directories('src/util'),
    include_directories('src/arch/aarch64'),
    include_directories(board_dir / 'include')
]

microkit_lib = c.find_library('microkit', dirs : board_dir / 'lib')
microkit_ld = board_dir / 'lib' / 'microkit.ld'

libvmm = static_library('vmm',
                sources : libvmm_src,
                include_directories : libvmm_includes,
                c_args : libvmm_defines,
                dependencies : microkit_lib)

package_guest_images = files('tools/package_guest_images.S')

subdir('examples/simple')
