from abc import ABC, abstractmethod

class CutterPlugin(ABC):
    name = ''
    description = ''
    version = ''
    author = ''
    core = None
    dockable = None

    @abstractmethod
    def __init__(self):
        pass

    @abstractmethod
    def setupPlugin(self):
        pass

    @abstractmethod
    def setupInterface(self, main, action):
        pass
