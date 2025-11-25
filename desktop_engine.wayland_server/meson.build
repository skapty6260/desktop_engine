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

dmabuf_protocol = wayland_protocols.get_variable('pkgdatadir') / 'unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml'

if fs.exists(dmabuf_protocol)
  add_project_arguments('-DHAVE_LINUX_DMABUF', language: 'c')
  add_project_arguments('-DHAVE_LINUX_DMABUF_UNSTABLE_V1', language: 'c')
  message('Linux DMA-BUF protocol found: yes')
else
  message('Linux DMA-BUF protocol not found')
endif

subdir('protocols')

executable('desktop_engine_wayland_server',
    sources: [
        'src/main.c',
        'src/server/server.c',
        'src/server/compositor.c',
        'src/server/shm.c',
        'src/server/xdg.c',
        'src/logger/logger.c',
        'src/config/config.c',
        wl_protos_src,
    ],
    dependencies: [wayland_server, wayland_protocols],
    include_directories: include_directories('protocols'),
    install: true
)