dbus_module_lib = static_library('desktopengine_dbus_module_lib', 
    'dbus_module_lib.c',
    dependencies: [
        dependency('dbus-1'),
    ],
    install: true,
    install_dir: get_option('libdir')
)

# Install header files
install_headers('dbus_module_lib.h',
  subdir: 'desktopengine_dbus_module_lib'
)

# PKG-config file
pkg = import('pkgconfig')
pkg.generate(
  name: 'desktopengine_dbus_module_lib',
  description: 'D-Bus module library for desktop engine',
  version: meson.project_version(),
  libraries: ['-L${libdir}', '-ldbus-module'],
  libraries_private: ['-ldbus-1'],
  requires_private: ['dbus-1'],
  subdirs: 'dbus_lib',
  filebase: 'desktopengine_dbus_module_lib'
)