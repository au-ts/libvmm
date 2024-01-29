project('libvmm', 'c')

src_libvmm = [
    'src/guest.c',
]

src_util = [
    'src/util/printf.c',
    'src/util/util.c',
]

src_aarch64 = [
    'src/arch/aarch64/fault.c',
    'src/arch/aarch64/linux.c',
    'src/arch/aarch64/psci.c',
    'src/arch/aarch64/smc.c',
    'src/arch/aarch64/tcb.c',
    'src/arch/aarch64/vcpu.c',
    'src/arch/aarch64/virq.c',
    'src/arch/aarch64/vgic/vgic_v2.c',
    # 'src/arch/aarch64/vgic/vgic_v3.c',
    'src/arch/aarch64/vgic/vgic.c',
]

# src_virtio = [
#     'src/virtio/console.c',
#     'src/virtio/mmio.c',
# ]

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

libvmm = library('libvmm',
                 sources :
                    src_util + src_aarch64 +
                    src_libvmm,
                include_directories : libvmm_includes,
                c_args : libvmm_defines)

package_guest_images = files('tools/package_guest_images.S')

subdir('examples/simple')
