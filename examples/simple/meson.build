# All variables are global so we might need to move / rename these.
microkit = find_program(get_option('microkit') / 'bin/microkit')
dtc = find_program('dtc')

system_desc = 'board' / board / 'simple.system'
image_dir = 'board' / board
linux = image_dir / 'linux'
dts = image_dir / 'linux.dts'
initrd = image_dir / 'rootfs.cpio.gz'

dtb = custom_target('simple-dtb',
                    input : dts,
                    output : 'linux.dtb',
                    command : [
                        dtc, '-q', '-I', 'dts', '-O', 'dtb',
                        '-o', '@OUTPUT@', '@INPUT@'
                    ])

dtb_path = dtb.full_path()
simple = executable('vmm.elf',
                    ['vmm.c', package_guest_images],
                    link_with : libvmm,
                    c_args : [
                        f'-DGUEST_KERNEL_IMAGE_PATH="@linux@"',
                        f'-DGUEST_DTB_IMAGE_PATH="@dtb_path@"',
                        f'-DGUEST_INITRD_IMAGE_PATH="@initrd@"',
                    ] + libvmm_defines,
                    link_args : ['-T' + microkit_ld],
                    link_depends : [microkit_ld, linux, dtb, initrd],
                    include_directories : libvmm_includes)

microkit_cmd = [
    microkit, meson.current_source_dir() / system_desc,
    # TODO: Allow elfs to be passed in directly to Microkit
    '--search-path', meson.current_build_dir(), image_dir,
    '--board', board,
    '--config', buildtype,
    '-o', '@OUTPUT0@',
    '-r', '@OUTPUT1@']

custom_target('simple',
              input : simple,
              output : ['loader.img', 'report.txt'],
              command : microkit_cmd,
              build_by_default : true)
