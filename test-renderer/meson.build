project('dbus-buffer-client', 'c',
  default_options: ['warning_level=3']
)

dbus_dep = dependency('dbus-1')

executable('dbus-buffer-client',
  'buffer_monitor.c',
  dependencies: [dbus_dep],
  install: true
)