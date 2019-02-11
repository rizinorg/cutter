
import os
import xml.etree.ElementTree as et
import sys

script_path = os.path.dirname(os.path.realpath(__file__))

ts_tree = et.parse(os.path.join(script_path, "bindings.xml"))
ts_root = ts_tree.getroot()

package = ts_root.attrib["package"]

type_tags = ["object-type", "value-type"]
types = [child.attrib["name"] for child in ts_root if child.tag in type_tags]

cpp_files_gen = [f"{package.lower()}_module_wrapper.cpp"]
cpp_files_gen.extend([f"{typename.lower()}_wrapper.cpp" for typename in types])

cpp_files_gen = [os.path.join(sys.argv[1], package, f) for f in cpp_files_gen]
sys.stdout.write(";".join(cpp_files_gen))
