project('desktop_engine_wayland_server', 'c',
    version: '1.0.0',
    license: 'GPL',
    default_options: [
        'c_std=c11',
        'warning_level=3',
        'werror=true'
    ]
)

add_project_arguments(
    [
        '-D_POSIX_C_SOURCE=200809L',

        '-Wno-unused-parameter',
        '-Wno-unused-variable',
        '-Wno-unused-function',
        '-Wno-unused-but-set-variable',
        '-Wno-maybe-uninitialized',
    ],
    language: 'c'
)

wayland_server = dependency('wayland-server', required: true)
wayland_protocols = dependency('wayland-protocols', required: true)
dbus = dependency('dbus-1', required: true)

subdir('protocols')

# Include dbus modules
#subdir('src/dbus-modules/test-module')

src_files = [
    'src/main.c',
    'src/wayland/server.c',
    'src/wayland/compositor.c',
    'src/wayland/compositor_surface_dummies.c',
    'src/wayland/shm.c',
    'src/wayland/buffer.c',
    'src/xdg-shell/xdg.c',
    'src/logger.c',
    'src/config.c',
    'src/dbus-server/server.c',
    # test_module_sources,
    wl_protos_src,
]

include_files = [
    include_directories('protocols'), 
    include_directories('include'), 
    #include_directories('src/dbus-modules/test-module')
]

executable('desktop_engine_wayland_server',
    sources: src_files,
    dependencies: [wayland_server, wayland_protocols, dbus, 
    #test_module_dep
    ],
    include_directories: include_files,
    install: true
)

# Дополнительный исполняемый файл для тестирования D-Bus
#executable('test-dbus-client',
#    sources: [
#        'src/dbus-modules/test-module/test-client.c',
#    ],
#    dependencies: [dbus],
#    include_directories: [
#        include_directories('include'),
#    ],
#    install: false
#)