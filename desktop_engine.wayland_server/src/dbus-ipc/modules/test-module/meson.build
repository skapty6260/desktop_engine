# Модуль для тестирования D-Bus

# Генерация кода из XML (опционально)
# gdbus_codegen = find_program('gdbus-codegen')
# 
# generated = custom_target(
#     'test-module-generated',
#     input: 'test-module.xml',
#     output: ['test-module-generated.c', 'test-module-generated.h'],
#     command: [
#         gdbus_codegen,
#         '--generate-c-code', 'test_module',
#         '--c-namespace', 'TestModule',
#         '--interface-prefix', 'org.skapty6260.DesktopEngine.',
#         '@INPUT@'
#     ]
# )

# Исходные файлы модуля
test_module_sources = files(
    'test-module.c'
)

# Заголовочные файлы
test_module_headers = files(
    'test-module.h'
)

# Тестовый клиент
test_client_source = files(
    'test-client.c'
)

# Библиотека модуля (статическая)
test_module_lib = static_library(
    'test-module',
    test_module_sources,
    dependencies: [
        dependency('dbus-1'),
    ],
    include_directories: [
        include_directories('../../../../include'),
        include_directories('../../..'),
    ],
    c_args: [
        '-DTEST_MODULE_VERSION="1.0.0"',
    ],
    install: false
)

# Экспортируем библиотеку для использования в основном проекте
test_module_dep = declare_dependency(
    link_with: test_module_lib,
    include_directories: include_directories('.'),
    sources: test_module_headers,
)

# Экспортируем переменные для основного meson.build
test_module_sources = test_module_sources
test_module_dep = test_module_dep