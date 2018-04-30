import argparse
import importlib.util
import logging
import os
import pprint
import subprocess
import sys

VARS = {'QT':[], 'SOURCES':[], 'HEADERS':[], 'FORMS':[], 'RESOURCES':[],
        'VERSION':[], 'ICON':[]}

ROOT = None
log = None
r2_meson_mod = None

def import_r2_meson_mod():
    """Import radare2/sys/meson.py"""
    global r2_meson_mod
    folder = os.path.dirname(__file__)
    r2_meson_path = os.path.join(folder, 'radare2', 'sys', 'meson.py')
    r2_meson_spec = importlib.util.spec_from_file_location('meson', r2_meson_path)
    r2_meson_mod = importlib.util.module_from_spec(r2_meson_spec)
    r2_meson_spec.loader.exec_module(r2_meson_mod)

def set_global_vars():
    global log
    global ROOT

    ROOT = os.path.abspath(os.path.dirname(__file__))

    logging.basicConfig(format='[%(name)s][%(levelname)s]: %(message)s',
                        level=logging.DEBUG)
    log = logging.getLogger('cutter-meson')
    log.debug('ROOT: %s', ROOT)

    r2_meson_mod.set_global_variables()

def parse_qmake_file():
    log.info('Parsing qmake file')
    with open(os.path.join(ROOT, 'src', 'Cutter.pro')) as qmake_file:
        lines = qmake_file.readlines()
    var_name = None
    end_of_def = True
    for line in lines:
        words = line.split()
        if not words:
            continue
        if words[0].startswith('#'):
            continue
        if not var_name and words[0] in VARS:
            var_name = words[0]
            words = words[2:]
        if not var_name:
            continue
        end_of_def = words[-1] != '\\'
        if not end_of_def:
            words = words[:-1]
        for word in words:
            VARS[var_name].append(word)
        if end_of_def:
            var_name = None
    qt_mod_translation = { "webenginewidgets": "WebEngineWidgets" }
    VARS['QT'] = list(map(lambda s: qt_mod_translation.get(s, str.title(s)), VARS['QT']))
    log.debug('Variables: \n%s', pprint.pformat(VARS, compact=True))

def win_dist(args):
    build = os.path.join(ROOT, args.dir)
    dist = os.path.join(ROOT, args.dist)
    os.makedirs(dist)
    r2_meson_mod.copy(os.path.join(build, 'Cutter.exe'), dist)
    log.debug('Deploying Qt5')
    subprocess.call(['windeployqt', '--release', os.path.join(dist, 'Cutter.exe')])
    log.debug('Deploying libr2')
    r2_meson_mod.PATH_FMT.update(r2_meson_mod.R2_PATH)
    r2_meson_mod.win_dist_libr2(DIST=dist, BUILDDIR=os.path.join(build, 'subprojects', 'radare2'),
                                R2_DATDIR=r'radare2\share', R2_INCDIR=r'radare2\include')

def build(args):
    cutter_builddir = os.path.join(ROOT, args.dir)
    if not args.webengine:
        VARS['QT'].remove('WebEngineWidgets')
    if not os.path.exists(cutter_builddir):
        defines = []
        defines.append('-Dversion=%s' % VARS['VERSION'][0])
        defines.append('-Dqt_modules=%s' % ','.join(VARS['QT']))
        defines.append('-Dsources=%s' % ','.join(VARS['SOURCES']))
        defines.append('-Dheaders=%s' % ','.join(VARS['HEADERS']))
        defines.append('-Dui_files=%s' % ','.join(VARS['FORMS']))
        defines.append('-Dqresources=%s' % ','.join(VARS['RESOURCES']))
        defines.append('-Denable_jupyter=%s' % str(args.jupyter).lower())
        defines.append('-Denable_webengine=%s' % str(args.webengine).lower())
        if os.name == 'nt':
            defines.append('-Dradare2:r2_incdir=radare2/include')
            defines.append('-Dradare2:r2_libdir=radare2/lib')
            defines.append('-Dradare2:r2_datdir=radare2/share')
        r2_meson_mod.meson(os.path.join(ROOT, 'src'), cutter_builddir,
                           prefix=cutter_builddir, backend=args.backend,
                           release=True, shared=False, options=defines)
    log.info('Building cutter')
    if args.backend == 'ninja':
        r2_meson_mod.ninja(cutter_builddir)
    else:
        project = os.path.join(cutter_builddir, 'Cutter.sln')
        r2_meson_mod.msbuild(project, '/m')

def create_sp_dir():
    sp_dir = os.path.join(ROOT, 'src', 'subprojects')
    sp_r2_dir = os.path.join(sp_dir, 'radare2')
    if not os.path.exists(sp_r2_dir):
        os.makedirs(sp_dir, exist_ok=True)
        r2_dir = os.path.join(ROOT, 'radare2')
        try:
            os.symlink(r2_dir, sp_r2_dir, target_is_directory=True)
        except OSError as e:
            log.error('%s', e)
            if os.name == 'nt':
                log.info('Execute command as Administrator:\n'
                          'MKLINK /D "%s" "%s"', sp_r2_dir, r2_dir)
            sys.exit(1)

def main():
    set_global_vars()

    parser = argparse.ArgumentParser(description='Meson script for Cutter')
    parser.add_argument('--backend', choices=r2_meson_mod.BACKENDS,
                        default='ninja', help='Choose build backend')
    parser.add_argument('--dir', default='build',
                        help='Destination build directory')
    parser.add_argument('--jupyter', action='store_true',
                        help='Enable Jupyter support')
    parser.add_argument('--webengine', action='store_true',
                        help='Enable QtWebEngine support')
    if os.name == 'nt':
        parser.add_argument('--dist', help='dist directory')
    args = parser.parse_args()

    if hasattr(args, 'dist') and args.dist and os.path.exists(args.dist):
        log.error('%s already exists', args.dist)
        sys.exit(1)

    log.debug('Arguments: %s', args)

    create_sp_dir()
    parse_qmake_file()

    build(args)

    if hasattr(args, 'dist') and args.dist:
        win_dist(args)

import_r2_meson_mod()
if __name__ == '__main__':
    main()
