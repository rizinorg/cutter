import sys
import importlib
import _qtres

class QtResLoader:
    @classmethod
    def get_code(cls, name):
        return _qtres.get_code(name)

    @classmethod
    def create_module(cls, spec):
        return None

    @classmethod
    def exec_module(cls, module):
        code = cls.get_code(module.__name__)
        if code is None:
            raise ImportError("get_code() failed")
        exec(code, module.__dict__)

class QtResFinder:
    @classmethod
    def find_spec(cls, fullname, path=None, target=None):
        if path or target:
            return None
        if not _qtres.exists(fullname):
            return None
        return importlib._bootstrap.ModuleSpec(fullname, QtResLoader)

sys.meta_path.append(QtResFinder)
