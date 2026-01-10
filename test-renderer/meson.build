project('test-renderer_dbus-monitor', 'c',
    version: '1.0.0',
    license: 'GPL',
    default_options: [
        'c_std=c11',
        'warning_level=3',
        'werror=true'
    ]
)

vulkan = dependency('vulkan', required: true)
dbus = dependency('dbus-1', required: true)
glfw = dependency('glfw3', required: true)

executable('dbus-buffer-client',
  'buffer_monitor.c',
  dependencies: [dbus],
  install: true
)

include_files = [
    include_directories('include'), 
]

src_files = [
    'src/main.c',
    'src/window.c',
    'src/vulkan.c',
    'src/buffer_mgr.c',
    'src/vulkan_texture.c'
]

subdir('shaders')

executable('renderer',
    sources: src_files,
    dependencies: [vulkan, dbus, glfw],
    include_directories: include_files,
    link_depends: shader_sources
)