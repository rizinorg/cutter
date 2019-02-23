
import sys
import re
import json

in_filename = sys.argv[1]
out_filename = sys.argv[2]
vars = json.loads(sys.argv[3])

with open(in_filename, "r") as f:
    content = f.read()

content = content.replace("\\\"", "\"")

varname_pattern = re.compile("[A-Za-z0-9_]+")
for name, value in vars.items():
    if varname_pattern.fullmatch(name) is None:
        print("Name \"{}\" is not a valid variable name.".format(name))
        continue

    pattern = "\\$\\$({}|\\{{{}\\}})".format(name, name)
    print(pattern)
    content = re.sub(pattern, str(value), content)

with open(out_filename, "w") as f:
    f.write(content)
