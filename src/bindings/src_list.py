#!/bin/env python3

import os
import xml.etree.ElementTree as et
import sys


script_path = os.path.dirname(os.path.realpath(__file__))


def get_cpp_files_gen(args):
    ts_tree = et.parse(os.path.join(script_path, "bindings.xml"))
    ts_root = ts_tree.getroot()

    package = ts_root.attrib["package"]

    type_tags = ["object-type", "value-type"]
    types = [child.attrib["name"] for child in ts_root if child.tag in type_tags]

    cpp_files_gen = [f"{package.lower()}_module_wrapper.cpp"]
    cpp_files_gen.extend([f"{typename.lower()}_wrapper.cpp" for typename in types])

    if len(args) > 0:
        return [os.path.join(args[0], package, f) for f in cpp_files_gen]
    else:
        return [os.path.join(package, f) for f in cpp_files_gen]


def cmd_cmake(args):
    sys.stdout.write(";".join(get_cpp_files_gen(args)))


def cmd_qmake(args):
    sys.stdout.write("\n".join(get_cpp_files_gen(args)) + "\n")


cmds = {"cmake": cmd_cmake, "qmake": cmd_qmake}

if len(sys.argv) < 2 or sys.argv[1] not in cmds:
    print(f"""usage: {sys.argv[0]} [{"/".join(cmds.keys())}] [base path]""")
    exit(1)
cmds[sys.argv[1]](sys.argv[2:])
