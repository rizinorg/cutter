
bindings_xml = configure_file(input: 'bindings.xml',
                              output: 'bindings.xml',
                              copy: true)

conf_json = '''{"BINDINGS_SRC_DIR":"@0@",
                "BINDINGS_BUILD_DIR":"@1@",
                "BINDINGS_INCLUDE_DIRS":"@2@",
                "PYSIDE_TYPESYSTEMS":"@3@"}'''.format(
                meson.current_source_dir(),
                meson.current_build_dir(),
                ':'.join(bindings_generate_inc),
                pyside_dep.get_pkgconfig_variable('typesystemdir'))
bindings_txt = configure_file(input: 'bindings.txt.in',
               output: 'bindings.txt',
               command: configure_qmake_cmd + ['@INPUT@', '@OUTPUT0@', conf_json])

bindings_h = files('bindings.h')

subdir('CutterBindings')