resources = custom_target(
  'resources',
  output: 'resources.c',
  input: ['gresources.xml', 'css/style.css', 'gl/shader.frag', 'gl/shader.vert'],
  command: [
    find_program('glib-compile-resources'),
    '--target=@OUTPUT@',
    '--sourcedir=@CURRENT_SOURCE_DIR@',
    '--generate-source',
    '@CURRENT_SOURCE_DIR@/gresources.xml',
  ],
  build_by_default: true,
)