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
    ],
    language: 'c'
)

wayland_server = dependency('wayland-server', required: true)
wayland_protocols = dependency('wayland-protocols', required: true)

subdir('protocols')

executable('desktop_engine_wayland_server',
    sources: [
        'src/main.c',
        'src/server/server.c',
        'src/server/wayland/compositor.c',
        'src/server/wayland/compositor_surface_dummies.c',
        'src/server/wayland/shm.c',
        'src/server/wayland/buffer.c',
        'src/server/xdg-shell/xdg.c',
        'src/logger/logger.c',
        'src/config/config.c',
        'src/ipc/network_server.c',
        wl_protos_src,
    ],
    dependencies: [wayland_server, wayland_protocols],
    include_directories: [include_directories('protocols'), include_directories('src')],
    install: true
)