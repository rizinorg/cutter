import cutter
from cutter_plugin import CutterPlugin
from PySide2 import QtWidgets
from PySide2.QtCore import QObject, SIGNAL, Qt
from PySide2.QtGui import QFont
import CutterBindings

class CutterSamplePlugin(CutterPlugin):
    name = "SamplePlugin"
    description = "A sample plugin written in python."
    version = "1.0"
    author = "xarkes and thestr4ng3r :-P"

    def setupInterface(self):
        super().setupInterface()

        # Create dock widget and content widget
        dock_widget = QtWidgets.QDockWidget(self.main)
        dock_widget.setObjectName("FancyDockWidgetFromCoolPlugin")
        dock_widget.setWindowTitle("Test Widget")
        content = QtWidgets.QWidget()
        dock_widget.setWidget(content)

        # Create layout and label
        layout = QtWidgets.QVBoxLayout(dock_widget)
        content.setLayout(layout)
        self.text = QtWidgets.QLabel(content)
        self.text.setSizePolicy(QtWidgets.QSizePolicy.Preferred, QtWidgets.QSizePolicy.Preferred)
        layout.addWidget(self.text)

        button = QtWidgets.QPushButton(content)
        button.setText("Want a fortune?")
        button.setSizePolicy(QtWidgets.QSizePolicy.Maximum, QtWidgets.QSizePolicy.Maximum)
        button.setMaximumHeight(50)
        button.setMaximumWidth(200)
        layout.addWidget(button)
        layout.setAlignment(button, Qt.AlignHCenter)

        QObject.connect(CutterBindings.CutterCore.getInstance(), SIGNAL("seekChanged(RVA)"), self.generate_fortune)
        QObject.connect(button, SIGNAL("clicked()"), self.generate_fortune)

        return self.makeCppPointer(dock_widget)


    def generate_fortune(self):
        res = cutter.cmd("?E `fo`")
        self.text.setText(res)


# Instantiate our plugin
plugin = CutterSamplePlugin()
