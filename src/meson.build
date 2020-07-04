project('Cutter', 'cpp', default_options: 'cpp_std=c++11', meson_version: '>=0.47.0')

qt5_mod = import('qt5')
py3_exe = import('python').find_installation('python3')

qt_modules = []
feature_define_args = []
if get_option('enable_python')
  message('Python is enabled')
  feature_define_args += ['-DCUTTER_ENABLE_PYTHON']

  if get_option('enable_python_bindings')
    message('Python Bindings are enabled')
    feature_define_args += ['-DCUTTER_ENABLE_PYTHON_BINDINGS']
  endif
endif

add_project_arguments(feature_define_args, language: 'cpp')

parse_cmd = [py3_exe, join_paths(meson.current_source_dir(), '../scripts/meson_parse_qmake.py')]
configure_qmake_cmd = [py3_exe, join_paths(meson.current_source_dir(), '../scripts/meson_configure_qmake_in.py')]

qt_modules += run_command(parse_cmd + ['QT'], check: true).stdout().split(';')
sources = run_command(parse_cmd + ['SOURCES'], check: true).stdout().split(';')
headers = run_command(parse_cmd + ['HEADERS'], check: true).stdout().split(';')
ui_files = run_command(parse_cmd + ['FORMS'], check: true).stdout().split(';')
qresources = run_command(parse_cmd + ['RESOURCES'], check: true).stdout().split(';')
version_major = run_command(parse_cmd + ['CUTTER_VERSION_MAJOR'], check: true).stdout()
version_minor = run_command(parse_cmd + ['CUTTER_VERSION_MINOR'], check: true).stdout()
version_patch = run_command(parse_cmd + ['CUTTER_VERSION_PATCH'], check: true).stdout()

conf_json = '''{"CUTTER_VERSION_MAJOR":@0@,
                "CUTTER_VERSION_MINOR":@1@,
                "CUTTER_VERSION_PATCH":@2@}'''.format(
                version_major, version_minor, version_patch)
configure_file(input: 'CutterConfig.h.in',
               output: 'CutterConfig.h',
               command: configure_qmake_cmd + ['@INPUT@', '@OUTPUT0@', conf_json])
conf_inc = include_directories('.')

sp_dir = join_paths(meson.source_root(), 'subprojects')
sp_r2_dir = join_paths(sp_dir, 'radare2')
exists_cmd = '__import__("sys").exit(__import__("os").path.exists("@0@"))'.format(sp_r2_dir)
if run_command(py3_exe, '-c', exists_cmd).returncode() == 0
  r2_src_dir = join_paths(meson.source_root(), '..', 'radare2')
  if host_machine.system() == 'windows'
    sp_dir = '\\'.join(sp_dir.split('/'))
    sp_r2_dir = '\\'.join(sp_r2_dir.split('/'))
    r2_src_dir = '\\'.join(r2_src_dir.split('/'))
    link_cmd = ['CMD', '/C', 'MKDIR', sp_dir, '&', 'MKLINK', '/D', sp_r2_dir, r2_src_dir]
  else
    link_cmd = ['sh', '-c', 'mkdir @0@ ; ln -s @1@ @2@'.format(sp_dir, r2_src_dir, sp_r2_dir)]
  endif
  run_command(link_cmd, check: true)
endif

libr2_dep = dependency('libr2',
    fallback : ['radare2', 'libr2_dep'],
    default_options : ['enable_tests=false'])

qt5dep = dependency('qt5', modules: qt_modules, main: true)

deps = [libr2_dep, qt5dep]
if get_option('enable_python')
  py3_dep = dependency('python3')
  deps += [py3_dep]
  if get_option('enable_python_bindings')
    shiboken_dep = dependency('shiboken2', method: 'pkg-config')
    pyside_dep = dependency('pyside2', method: 'pkg-config')
    pyside_inc_dep = declare_dependency(include_directories: include_directories(join_paths(pyside_dep.get_pkgconfig_variable('includedir'), 'QtCore'),
                                                                                 join_paths(pyside_dep.get_pkgconfig_variable('includedir'), 'QtGui'),
                                                                                 join_paths(pyside_dep.get_pkgconfig_variable('includedir'), 'QtWidgets')))
    deps += [shiboken_dep, pyside_dep, pyside_inc_dep]
    shiboken_exe = shiboken_dep.get_pkgconfig_variable('generator_location')
    qt5core_dep = dependency('Qt5Core', method: 'pkg-config')
    bindings_generate_inc = [meson.current_source_dir(),
                             join_paths(meson.current_source_dir(), 'core'),
                             join_paths(meson.current_source_dir(), 'common'),
                             join_paths(meson.current_source_dir(), 'widgets'),
                             join_paths(meson.current_source_dir(), 'plugins'),
                             join_paths(meson.current_source_dir(), 'menus'),
                             join_paths(meson.current_source_dir(), 'subprojects/radare2/libr/include'),
                             join_paths(meson.current_build_dir(), 'subprojects/radare2'),
                             qt5core_dep.get_pkgconfig_variable('includedir'),
                             join_paths(qt5core_dep.get_pkgconfig_variable('includedir'), 'QtCore'),
                             join_paths(qt5core_dep.get_pkgconfig_variable('includedir'), 'QtGui'),
                             join_paths(qt5core_dep.get_pkgconfig_variable('includedir'), 'QtWidgets')]
    message('bindings_inc: @0@'.format(bindings_generate_inc))
    subdir('bindings')
    sources += bindings_target
  endif
endif

moc_files = qt5_mod.preprocess(
  moc_headers: headers,
  ui_files: ui_files,
  qresources: qresources,
  moc_extra_arguments: feature_define_args
)

cpp = meson.get_compiler('cpp')

if host_machine.system() == 'windows'
  add_project_arguments('-D_CRT_NONSTDC_NO_DEPRECATE', language: 'cpp')
  add_project_arguments('-D_CRT_SECURE_NO_WARNINGS', language: 'cpp')
  add_project_link_arguments(join_paths(py3_exe.get_variable('BINDIR'), 'libs', 'python3.lib'), language: 'cpp')
endif

cutter_exe = executable(
  'Cutter',
  moc_files,
  gui_app: true,
  sources: sources,
  include_directories: [
    include_directories('core', 'common', 'widgets', 'plugins'),
    conf_inc
  ],
  dependencies: deps,
)
