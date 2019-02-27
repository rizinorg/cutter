import json
from _cutter import *

try:
    from CutterBindings import *

    def core():
        return CutterCore.instance()
except ImportError:
    pass


def cmdj(command):
    """Execute a JSON command and return the result as a dictionary"""
    return json.loads(cmd(command))


