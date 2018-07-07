from abc import ABC, abstractmethod


class CutterPlugin(ABC):
    name = ''
    description = ''
    version = ''
    author = ''

    @abstractmethod
    def setupPlugin(self):
        pass

    @abstractmethod
    def setupInterface(self):
        pass

