import cutter
from cutter_plugin import CutterPlugin
from PySide2 import QtCore, QtWidgets

class CutterSamplePlugin(CutterPlugin):
    def setupPlugin(self):
        self.name = 'SamplePlugin'
        self.description = 'A sample plugin written in python.'
        self.version = '1.0'
        self.author = 'xarkes'
        self.app = QtCore.QCoreApplication.instance()

    def setupInterface(self):
        for _ in range(200):
            print('FUCK', end='')
        main_window = self.app.findChild('MainWindow')
        dock_widget = QtWidgets.QDockWidget(main_window)
        dock_widget.setWindowTitle('Test Widget!!!')
        for _ in range(200):
            print(main_window, dock_widget)
        return dock_widget


# Instantiate our plugin
plugin = CutterSamplePlugin()

