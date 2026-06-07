project('Binc Trader', 'c', version: '0.0.1')

webkitdep = dependency('webkitgtk-6.0')
gldep = dependency('gl')
glesdep = dependency('glesv2')
sqlitedep = dependency('sqlite3')
libjsondep = dependency('json-glib-1.0')
soupdep = dependency('libsoup-3.0')
soupdep = dependency('libsecret-1')

subdir('src')
subdir('res')

executable(
    'binctrader',
    sources,
    resources,
    dependencies: [
        webkitdep,
        gldep,
        glesdep,
        sqlitedep,
        libjsondep,
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
  dependencies : [libsoup_dep, json_dep, sqlite_dep],
  install : true
)