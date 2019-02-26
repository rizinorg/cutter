# -*- coding: utf-8 -*-
import os
import sys
import errno
import xml.etree.ElementTree


ALLOWED_TYPES = ['class', 'interface', 'struct', 'union']


def write_file(name, text, destdir):
    """Write the output file for module/package <name>."""
    fname = os.path.join(destdir, '%s.%s' % (name, 'rst'))

    if not os.path.exists(os.path.dirname(fname)):
        try:
            os.makedirs(os.path.dirname(fname))
        except OSError as exc:  # Guard against race condition
            if exc.errno != errno.EEXIST:
                raise
    try:
        with open(fname, 'r') as target:
            orig = target.read()
            if orig == text:
                return
    except FileNotFoundError:
        # Don't mind if it isn't there
        pass

    with open(fname, 'w') as target:
        target.write(text)


def format_heading(level, text):
    """Create a heading of <level> [1, 2 or 3 supported]."""
    underlining = ['=', '-', '~', ][level - 1] * len(text)
    return '%s\n%s\n\n' % (text, underlining)


def format_directive(package_type, package, project = None):
    """Create the breathe directive and add the options."""
    directive = '.. doxygen%s:: %s\n' % (package_type, package)
    if project:
        directive += '   :project: %s\n' % project
    return directive


def create_package_file(package, package_type, package_id, package_folder, rootpath, destdir):
    """Build the text of the file and write the file."""
    text = format_heading(1, '%s' % (package))
    text += format_directive(package_type, package)

    xmlfile = os.path.join(rootpath, package_id + '.xml')
    f = xml.etree.ElementTree.parse(os.path.join(xmlfile))

    write_file(os.path.join(package_folder, package_id), text, destdir)


def create_modules_toc_file(key, value, destdir):
    """Create the module's index."""
    text = format_heading(1, '%s' % value)
    text += '.. toctree::\n'
    text += '   :glob:\n\n'
    text += '   %s/*\n' % key

    write_file('%slist' % key, text, destdir)


def get_compound_folder(rootpath, compound):
    fxml = xml.etree.ElementTree.parse(os.path.join(rootpath, compound.get('refid')) + '.xml')
    loc = fxml.getroot()[0].find('location')
    dirname = os.path.basename(os.path.split(loc.get('file'))[0])
    return dirname


def recurse_tree(rootpath, destdir):
    """
    Look for every file in the directory tree and create the corresponding
    ReST files.
    """
    index = xml.etree.ElementTree.parse(os.path.join(rootpath, 'index.xml'))

    for compound in index.getroot():
        if compound.get('kind') not in ALLOWED_TYPES:
            continue
        create_package_file(compound.findtext('name'), compound.get('kind'),
                            compound.get('refid'), get_compound_folder(rootpath, compound),
                            rootpath, destdir)

def get_folders_tree(rootpath):
    tmp = []

    # Retrieve the subfolders indexes
    for root, _, files in os.walk(rootpath):
        for xmlfile in files:
            if not xmlfile.startswith('dir_'):
                continue
            tmp.append(xmlfile)

    # Iterate on them
    dirs = []
    for xmlfile in tmp:
        data = xml.etree.ElementTree.parse(os.path.join(rootpath, xmlfile))
        if not data:
            continue
        for compound in data.getroot():
            name = compound.findtext('compoundname')
            dirs.append(name)

    return dirs


def main():
    rootpath = './doxygen-out/xml'
    destdir = './source/api'

    if not os.path.exists(destdir):
        os.makedirs(destdir)

    dirs = sorted(get_folders_tree(rootpath))
    source_root = dirs[0]
    source_dirs = dirs[1:]
    out_dirs = [os.path.basename(d) for d in dirs]

    # TODO Handle only one level subfolders
    for key in out_dirs:
        ddir = os.path.join(destdir, key)
        if not os.path.exists(ddir):
            os.makedirs(ddir)
        create_modules_toc_file(key, key.capitalize(), destdir)
    recurse_tree(rootpath, destdir)


# So program can be started with "python -m breathe.apidoc ..."
if __name__ == "__main__":
    main()
