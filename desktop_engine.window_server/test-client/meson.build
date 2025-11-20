project('simple-wayland-client', 'c',
  version: '1.0.0',
  default_options: [
    'warning_level=3',
    'c_std=c11',
    'default_library=static'
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

# Зависимости
wayland_client = dependency('wayland-client')
wayland_protos = dependency('wayland-protocols', version: '>=1.14')
cairo = dependency('cairo')

# Генерация протоколов для клиента
wayland_scanner = find_program('wayland-scanner', native: true)

wl_protocol_dir = wayland_protos.get_variable('pkgdatadir')

protocols = [
  wl_protocol_dir / 'stable/xdg-shell/xdg-shell.xml',
]

client_protocols_src = []
client_protocols_headers = []

foreach p : protocols
  base_name = p.split('/')[-1].split('.')[0]
  
  # Генерируем client header
  client_protocols_headers += custom_target(
    base_name + '_client_header',
    input: p,
    output: '@BASENAME@-client-protocol.h',
    command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
    build_by_default: true,
  )
  
  # Генерируем private code
  client_protocols_src += custom_target(
    base_name + '_client_code',
    input: p,
    output: '@BASENAME@-protocol.c',
    command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
    build_by_default: true,
  )
endforeach

# Исполняемый файл
executable('test-client', 
  sources: [
    'client.c',
    client_protocols_src,
  ],
  dependencies: [wayland_client, cairo],
  include_directories: include_directories('.'),
  install: true,
)