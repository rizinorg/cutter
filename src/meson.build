#TODO: icon
project('cutter', 'cpp', default_options: 'cpp_std=c++11')

#TODO: add console option
console = false

qt5_mod = import('qt5')

version = get_option('version')

qt_modules = get_option('qt_modules')
sources = get_option('sources')
headers = get_option('headers')
ui_files = get_option('ui_files')
qresources = get_option('qresources')

qt5dep = dependency('qt5', modules: qt_modules)

moc_files = qt5_mod.preprocess(
  moc_headers: headers,
  ui_files: ui_files,
  qresources: qresources
)

cpp = meson.get_compiler('cpp')

platform_inc = []
if host_machine.system() == 'windows'
  add_project_arguments('-D_CRT_NONSTDC_NO_DEPRECATE', language: 'cpp')
  add_project_arguments('-D_CRT_SECURE_NO_WARNINGS', language: 'cpp')
  platform_inc = include_directories('../cutter_win32/include')
  if not console
    # Workaround for https://github.com/mesonbuild/meson/issues/2327
    qt_lib = run_command('qmake', '-query', 'QT_HOST_LIBS').stdout().strip()
    add_project_link_arguments('@0@/qtmain.lib'.format(qt_lib), language: 'cpp')
  endif
endif

add_project_arguments('-DAPP_VERSION="@0@"'.format(version), language: 'cpp')

r2 = subproject('radare2')
libr2_dep = r2.get_variable('libr2_dep')

cutter_exe = executable(
  'Cutter',
  moc_files,
  gui_app: not console,
  sources: sources,
  include_directories: platform_inc,
  dependencies: [libr2_dep, qt5dep],
)
