import cutter
from cutter_plugin import CutterPlugin, cutter_core
from PySide2 import QtWidgets
from PySide2.QtCore import QObject, SIGNAL, Qt, Slot

class CutterSamplePlugin(CutterPlugin):
    name = "SamplePlugin"
    description = "A sample plugin written in python."
    version = "1.0"
    author = "xarkes"

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

        QObject.connect(cutter_core, SIGNAL("seekChanged(RVA)"), self.on_seek_changed)
        QObject.connect(cutter_core, SIGNAL("refreshAll()"), self.on_refresh)
        QObject.connect(button, SIGNAL("clicked()"), self.on_button_clicked)

        return self.makeCppPointer(dock_widget)

    @Slot()
    def on_button_clicked(self):
        res = cutter.cmd("?E `fo`")
        self.text.setText(res)

    @Slot(int)
    def on_seek_changed(self, addr):
        res = cutter.cmd("?E addr=" + hex(addr))
        self.text.setText(res)

    @Slot()
    def on_refresh(self):
        self.text.clear()

# Instantiate our plugin
plugin = CutterSamplePlugin()
