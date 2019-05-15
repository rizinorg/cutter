import argparse
import importlib.util
import logging
import os
import subprocess
import sys

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
    if not os.path.exists(cutter_builddir):
        defines = ['-Denable_python=%s' % str(args.python).lower(),
                   '-Denable_python_bindings=%s' % str(args.python_bindings).lower()]
        if os.name == 'nt':
            defines.append('-Dradare2:r2_incdir=radare2/include')
            defines.append('-Dradare2:r2_libdir=radare2/lib')
            defines.append('-Dradare2:r2_datdir=radare2/share')
            defines.append('-Dc_args=-D_UNICODE -DUNICODE')
        r2_meson_mod.meson(os.path.join(ROOT, 'src'), cutter_builddir,
                           prefix=cutter_builddir, backend=args.backend,
                           release=args.release, shared=False, options=defines)
    if not args.nobuild:
        log.info('Building cutter')
        if args.backend == 'ninja':
            r2_meson_mod.ninja(cutter_builddir)
        else:
            project = os.path.join(cutter_builddir, 'Cutter.sln')
            r2_meson_mod.msbuild(project, '/m')

def main():
    set_global_vars()

    parser = argparse.ArgumentParser(description='Meson script for Cutter')
    parser.add_argument('--backend', choices=r2_meson_mod.BACKENDS,
                        default='ninja', help='Choose build backend')
    parser.add_argument('--dir', default='build',
                        help='Destination build directory')
    parser.add_argument('--python', action='store_true',
                        help='Enable Python support')
    parser.add_argument('--python-bindings', action='store_true',
                        help='Enable Python Bindings')
    parser.add_argument('--release', action='store_true',
                        help='Set the build as Release (remove debug info)')
    parser.add_argument('--nobuild', action='store_true',
                        help='Only run meson and do not build.')
    if os.name == 'nt':
        parser.add_argument('--dist', help='dist directory')
    args = parser.parse_args()

    if hasattr(args, 'dist') and args.dist and os.path.exists(args.dist):
        log.error('%s already exists', args.dist)
        sys.exit(1)

    log.debug('Arguments: %s', args)

    build(args)

    if hasattr(args, 'dist') and args.dist:
        win_dist(args)

import_r2_meson_mod()
if __name__ == '__main__':
    main()
