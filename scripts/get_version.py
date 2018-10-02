
import os
import re

scripts_path = os.path.dirname(os.path.realpath(__file__))
pro_file_path = os.path.join(scripts_path, "..", "src", "Cutter.pro")

with open(pro_file_path, "r") as f:
    pro_content = f.read()

def version_var_re(name):
    return "^[ \t]*{}[ \t]*=[ \t]*(\d)+[ \t]*$".format(name)

m = re.search(version_var_re("CUTTER_VERSION_MAJOR"), pro_content, flags=re.MULTILINE)
version_major = int(m.group(1)) if m is not None else 0

m = re.search(version_var_re("CUTTER_VERSION_MINOR"), pro_content, flags=re.MULTILINE)
version_minor = int(m.group(1)) if m is not None else 0

m = re.search(version_var_re("CUTTER_VERSION_PATCH"), pro_content, flags=re.MULTILINE)
version_patch = int(m.group(1)) if m is not None else 0

print("{}.{}.{}".format(version_major, version_minor, version_patch))
