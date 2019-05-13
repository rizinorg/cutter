
import cutter

from PySide2.QtCore import Qt
from PySide2.QtWidgets import QAction, QVBoxLayout, QLabel, QWidget, QSizePolicy, QPushButton


class FortuneWidget(cutter.CutterDockWidget):
    def __init__(self, parent, action):
        super(FortuneWidget, self).__init__(parent, action)
        self.setObjectName("FancyDockWidgetFromCoolPlugin")
        self.setWindowTitle("Sample Python Plugin")

        content = QWidget()
        self.setWidget(content)

        # Create layout and label
        layout = QVBoxLayout(content)
        content.setLayout(layout)
        self.text = QLabel(content)
        self.text.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred)
        self.text.setFont(cutter.Configuration.instance().getFont())
        layout.addWidget(self.text)

        button = QPushButton(content)
        button.setText("Want a fortune?")
        button.setSizePolicy(QSizePolicy.Maximum, QSizePolicy.Maximum)
        button.setMaximumHeight(50)
        button.setMaximumWidth(200)
        layout.addWidget(button)
        layout.setAlignment(button, Qt.AlignHCenter)

        button.clicked.connect(self.generate_fortune)
        cutter.core().seekChanged.connect(self.generate_fortune)

        self.show()

    def generate_fortune(self):
        fortune = cutter.cmd("fo").replace("\n", "")
        res = cutter.core().cmdRaw(f"?E {fortune}")
        self.text.setText(res)


class CutterSamplePlugin(cutter.CutterPlugin):
    name = "SamplePlugin"
    description = "A sample plugin written in python."
    version = "1.0"
    author = "xarkes and thestr4ng3r :-P"

    def __init__(self):
        super(CutterSamplePlugin, self).__init__()

    def setupPlugin(self):
        pass

    def setupInterface(self, main):
        action = QAction("Sample Python Plugin", main)
        action.setCheckable(True)
        widget = FortuneWidget(main, action)
        main.addPluginDockWidget(widget, action)

    def terminate(self): # optional
        print("CutterSamplePlugin shutting down")


# This function will be called by Cutter and should return an instance of the plugin.
def create_cutter_plugin():
    return CutterSamplePlugin()
