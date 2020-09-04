    project('pwswdpp',
        [ 'c', 'cpp' ],
        license: [ 'GPLv2' ],
        default_options: [ 'c_std=c11', 'cpp_std=c++2a', 'b_asneeded=false', 'b_lundef=false'],
        version: '1.0.0'
    )

# Make sure the project gets cross compiled
    assert(meson.is_cross_build(), 'This project can only be cross-compiled. Make sure to call meson with the "--cross-file=dingux" option!')


# Available portlib libraries 
    dingux_libraries = meson.get_cross_property('dingux_libraries')


    libSDL = declare_dependency(link_args : [ '-L' + dingux_libraries + '/SDL/lib', '-lSDL' ],
                                include_directories : [ dingux_libraries + '/SDL/include' ])

    libSDL2 = declare_dependency(link_args : [ '-L' + dingux_libraries + '/SDL2/lib', '-lSDL2' ],
                                include_directories : [ dingux_libraries + '/SDL2/include' ])

    libdingux = declare_dependency(link_args : [ '-L' + dingux_libraries + '/libdingux/lib', '-ldingux' ],
                                include_directories : [ dingux_libraries + '/libdingux/include' ])

    libstdcpp = declare_dependency(link_args : [ '-L' + meson.get_cross_property('sys_root') + '/lib/libc.a', '-static-libstdc++', '-static-libgcc' ])

    linux_headers = meson.get_cross_property('sys_root') + '/usr/include'


# Source files and include directories
    source_files = [
        'source/main.cpp',
    ]

    include_dirs = include_directories('include', linux_headers)


# Executable building
    application = executable(
        'pwswdpp',
        source_files,
        native: false,
        name_prefix: '',
        dependencies: [ libstdcpp ],
        include_directories: include_dirs
    )

    # OPK creation
    custom_target(meson.project_name() + '.opk',
                build_by_default: true,
                input: [ application, 'installer/default.gcw0.desktop', 'installer/icon.png', 'installer/install.sh', 'installer/S92pwswdpp.sh' ],
                output: meson.project_name() + '.opk',
                command: [ meson.get_cross_property('opk_scripts') + '/build_opk', '@OUTPUT@', '@INPUT@' ]
                )

# Install target
    meson.add_install_script(meson.get_cross_property('opk_scripts') + '/install_opk', meson.current_build_dir() + '/' + meson.project_name() + '.opk')