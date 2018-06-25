from abc import ABC, abstractmethod

class CutterPlugin(ABC):
    name = ''
    description = ''
    version = ''
    author = ''
    app = None

    @abstractmethod
    def setupPlugin(self):
        pass

    @abstractmethod
    def setupInterface(self):
        pass

