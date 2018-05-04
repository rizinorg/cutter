import compileall
import os

root = os.path.abspath(os.path.join(os.path.dirname(__file__), os.pardir))
compileall.compile_dir(os.path.join(root, 'src', 'python'), legacy=True, optimize=2)
with open(os.path.join(root, 'src', 'resources.qrc'), 'r+b') as f:
    data = f.read()
    data = data.replace(b'.py<', b'.pyc<')
    f.seek(0)
    f.write(data)
