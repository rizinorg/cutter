from PySide2 import QtCore, QtWidgets, shiboken2

cutter_core = None

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

def set_cutter_core(addr):
    global cutter_core
    cutter_core = shiboken2.wrapInstance(addr, QtCore.QObject)
