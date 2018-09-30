import json
from _cutter import *

def cmdj(command):
    '''Execute a JSON command and return the result as a dictionnary'''
    return json.loads(cmd(command))
