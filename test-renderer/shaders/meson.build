glslc = find_program('glslc', required: true)

vertex_shader = custom_target(
  'vertex_shader',
  output: 'vertex.vert.spv',
  input: 'vertex.vert',
  command: [glslc, '@INPUT@', '-o', '@OUTPUT@'],
  install: false,
  build_by_default: true
)

fragment_shader = custom_target(
  'fragment_shader',
  output: 'fragment.frag.spv',
  input: 'fragment.frag',
  command: [glslc, '@INPUT@', '-o', '@OUTPUT@'],
  install: false,
  build_by_default: true
)

shader_sources = [vertex_shader, fragment_shader]