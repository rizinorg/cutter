#TODO: icon
project('Cutter', 'cpp', default_options: 'cpp_std=c++11')

feature_define_args = []
if get_option('enable_jupyter')
  message('Jupyter support enabled')
  add_project_arguments('-DCUTTER_ENABLE_JUPYTER', language: 'cpp')
  feature_define_args += ['-DCUTTER_ENABLE_JUPYTER']
  if get_option('enable_webengine')
    message('QtWebEngine support enabled')
    add_project_arguments('-DCUTTER_ENABLE_QTWEBENGINE', language: 'cpp')
    feature_define_args += ['-DCUTTER_ENABLE_QTWEBENGINE']
  endif
endif

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
  qresources: qresources,
  moc_extra_arguments: feature_define_args
)

cpp = meson.get_compiler('cpp')

platform_inc = []
if host_machine.system() == 'windows'
  add_project_arguments('-D_CRT_NONSTDC_NO_DEPRECATE', language: 'cpp')
  add_project_arguments('-D_CRT_SECURE_NO_WARNINGS', language: 'cpp')
  platform_inc = include_directories('../radare2/libr/include/msvc')
  # Workaround for https://github.com/mesonbuild/meson/issues/2327
  qt_lib = run_command('qmake', '-query', 'QT_HOST_LIBS').stdout().strip()
  add_project_link_arguments(join_paths(qt_lib, 'qtmain.lib'), language: 'cpp')
endif

add_project_arguments('-DAPP_VERSION="@0@"'.format(version), language: 'cpp')

r2 = subproject('radare2')
libr2_dep = r2.get_variable('libr2_dep')

deps = [libr2_dep, qt5dep]
if get_option('enable_jupyter')
  deps += [dependency('python3')]
endif

cutter_exe = executable(
  'Cutter',
  moc_files,
  gui_app: true,
  sources: sources,
  include_directories: platform_inc,
  dependencies: deps,
)
