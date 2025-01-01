project('Binc Terminal', 'c', version: '0.0.1')

webkitdep = dependency('webkitgtk-6.0')
gstreamerdep = dependency('gstreamer-1.0')
epoxydep = dependency('epoxy')
sqlitedep = dependency('sqlite3')
libjsondep = dependency('json-glib-1.0')
soupdep = dependency('libsoup-3.0')

subdir('src')
subdir('res')

executable(
    'terminal',
    sources,
    resources,
    dependencies: [
        webkitdep,
        gstreamerdep,
        epoxydep,
        sqlitedep,
        libjsondep,
        soupdep,
    ],
    include_directories : 'include',
    c_args: ['-fstack-protector'],
    link_args: ['-lm'],
    install: true,
)
