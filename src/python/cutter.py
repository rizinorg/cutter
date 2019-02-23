import json
from _cutter import *
from CutterBindings import *


def cmdj(command):
    """Execute a JSON command and return the result as a dictionary"""
    return json.loads(cmd(command))


def core():
    return CutterCore.instance()
