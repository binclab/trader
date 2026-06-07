project('Binc Trader', 'c', version: '0.0.1')

sqlitedep = dependency('sqlite3')
jsondep = dependency('json-glib-1.0')
soupdep = dependency('libsoup-3.0')

subdir('src')
subdir('res')

executable(
    'binctrader',
    sources,
    resources,
    dependencies: [
        sqlitedep,
        jsondep,
        soupdep,
    ],
    include_directories : 'include',
    c_args: ['-fstack-protector'],
    link_args: ['-lm'],
    install: false,
    build_by_default : false,
)

executable(
  'server',
  'src/server.c',
  dependencies : [soupdep, sqlitedep, jsondep],
  install : true
)