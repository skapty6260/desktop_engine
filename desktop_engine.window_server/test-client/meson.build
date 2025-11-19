project('test_wayland_client', 'c',
    version: '1.0.0',
    license: 'GPL',
    default_options: [
        'c_std=c11',
        'warning_level=3',
        'werror=true'
    ]
)

wayland_client = dependency('wayland-client')

executable('test-client',
    sources: [
        'client.c',
    ],
    dependencies: [wayland_client],
    install: true
)