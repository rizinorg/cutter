import os
import sys

name = sys.argv[1]
value = []
root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
with open(os.path.join(root, 'src', 'Cutter.pro')) as f:
    text = f.read()
text = text.replace('\\\n', '')
nbraces = 0
for line in text.split('\n'):
    line = line.strip()
    if not line or all(char in line for char in ('{', '}')):
        continue
    if line.startswith('}'):
        nbraces -= 1
        continue
    if line.endswith('{'):
        nbraces += 1
        continue
    if nbraces > 0 or '=' not in line:
        continue
    words = line.split()
    if words[0] == name and '=' in words[1]:
        value.extend(words[2:])
if name == 'QT':
    value = [str.title(s) for s in value]
if not value:
    sys.exit(1)
print(';'.join(value), end='')
