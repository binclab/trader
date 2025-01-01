glslang_validator = find_program('glslangValidator', required: true)

fragmentshader = custom_target(
  'fragmentshader',
  output: 'fragment.spv',
  input: 'shader.frag',
  command: [
    glslang_validator,
    '-V',
    '-o', meson.project_source_root() + '/res/@OUTPUT@',
    '@INPUT@',
  ],
)

vertexshader = custom_target(
  'vertexshader',
  output: 'vertex.spv',
  input: 'shader.vert',
  command: [
    glslang_validator,
    '-V',
    '-o', meson.project_source_root() + '/res/@OUTPUT@',
    '@INPUT@',
  ],
)

shader_targets = [fragmentshader, vertexshader]
