project('desktop_engine_windows_server', 'c',
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

executable('desktop_engine_win_server',
    sources: [
        'src/main.c',
        'src/server.c',
        'logger/logger.c',
        'config/config.c',
        wl_protos_src,
    ],
    dependencies: [wayland_server, wayland_protocols],
    include_directories: include_directories('protocols'),
    install: true
)