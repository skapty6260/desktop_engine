project('simple-wayland-client', 'c',
  version: '1.0.0',
  default_options: [
    'warning_level=3',
    'c_std=c11',
    'default_library=static'
  ]
)

# Зависимости
wayland_client = dependency('wayland-client')
wayland_protos = dependency('wayland-protocols', version: '>=1.14')
cairo = dependency('cairo')

subdir('..protocols')

# Исполняемый файл
executable('simple-client',
  [
    'client.c',
    wl_protos_src,
  ],
  dependencies: [wayland_client, cairo, wayland_protos],
  include_directories: include_directories('../protocols'),
  install: true,
)