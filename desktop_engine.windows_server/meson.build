project('desktop_engine_windows_server', 'c',
    version: '1.0.0',
    license: 'GPL',
    default_options: [
        'c_std=c11',
        'warning_level=3',
        'werror=true'
    ]
)

wayland_server = dependency('wayland-server', required: true)
# wayland_protocols = dependency('wayland_protocols', required: true)

executable('desktop_engine_win_server',
    sources: [
        'src/main.c',
        'logger/logger.c'
    ],
    include_directories: inc_dir,
    dependencies: [wayland_server],
    install: true
)