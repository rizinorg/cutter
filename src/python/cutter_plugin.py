from PySide2 import QtCore, QtWidgets
import shiboken2


class CutterPlugin(object):
    name = ''
    description = ''
    version = ''
    author = ''

    def setupPlugin(self):
        self.app = QtCore.QCoreApplication.instance()

    def setupInterface(self):
        for widget in QtWidgets.QApplication.topLevelWidgets():
            if widget.objectName() == "MainWindow":
                self.main = widget
                break

    def makeCppPointer(self, widget):
        ptr = shiboken2.getCppPointer(widget)[0]
        return ptr
        


