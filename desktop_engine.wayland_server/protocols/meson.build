wl_protocol_dir = wayland_protocols.get_variable('pkgdatadir')

wayland_scanner_dep = dependency('wayland-scanner', native: true)
wayland_scanner = find_program(
	wayland_scanner_dep.get_variable('wayland_scanner'),
	native: true,
)

protocols = [
	[wl_protocol_dir, 'stable/xdg-shell/xdg-shell.xml'],
	[wl_protocol_dir, 'stable/tablet/tablet-v2.xml'],
	[wl_protocol_dir, 'staging/cursor-shape/cursor-shape-v1.xml'],
	[wl_protocol_dir, 'unstable/xdg-output/xdg-output-unstable-v1.xml'],
	[wl_protocol_dir, 'unstable/linux-dmabuf/linux-dmabuf-unstable-v1.xml'],
	# 'wlr-layer-shell-unstable-v1.xml',
	# 'idle.xml',
	# 'wlr-output-power-management-unstable-v1.xml',
]

wl_protos_src = []
wl_protos_headers = []

foreach p : protocols
	# Обрабатываем пути протоколов
	if p.length() == 2
		xml_path = p[0] / p[1]
		base_name = p[1].split('/')[-1].split('.')[0]
	else
		xml_path = p
		base_name = p.split('/')[-1].split('.')[0]
	endif

	# Генерируем исходные файлы протоколов
	wl_protos_src += custom_target(
		base_name + '_protocol_c',
		input: xml_path,
		output: '@BASENAME@-protocol.c',
		command: [wayland_scanner, 'private-code', '@INPUT@', '@OUTPUT@'],
		build_by_default: true,
	)

	# Генерируем server headers
	wl_protos_headers += custom_target(
		base_name + '_server_h',
		input: xml_path,
		output: '@BASENAME@-protocol.h',
		command: [wayland_scanner, 'server-header', '@INPUT@', '@OUTPUT@'],
		build_by_default: true,
	)

	# Генерируем client headers (для полноты)
	custom_target(
		base_name + '_client_h',
		input: xml_path,
		output: '@BASENAME@-client-protocol.h',
		command: [wayland_scanner, 'client-header', '@INPUT@', '@OUTPUT@'],
		build_by_default: true,
	)
endforeach

foreach p : protocols
    if p[1].contains('linux-dmabuf')
        add_project_arguments('-DHAVE_LINUX_DMABUF', language: 'c')
        break
    endif
endforeach