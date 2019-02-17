
# 😠 https://github.com/mesonbuild/meson/issues/2320

bindings_src_files = run_command([py3_exe,
                                  join_paths(meson.current_source_dir(), '../src_list.py'),
                                  'meson']).stdout().split(';')

bindings_target = custom_target('bindings',
                                input: bindings_txt,
                                depend_files: [bindings_h, bindings_xml],
                                output: bindings_src_files,
                                command: [shiboken_exe, '--project-file=@INPUT@'])