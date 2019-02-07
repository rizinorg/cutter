
import cutter
from cutter_plugin import CutterPlugin
import CutterBindings

from PySide2.QtCore import QObject, SIGNAL, Qt
from PySide2.QtWidgets import QAction, QVBoxLayout, QLabel, QWidget, QSizePolicy, QPushButton


class FortuneWidget(CutterBindings.CutterDockWidget):
    def __init__(self, main, action):
        super(FortuneWidget, self).__init__(main, action)
        self.setObjectName("FancyDockWidgetFromCoolPlugin")
        self.setWindowTitle("Sample Python Plugin")

        content = QWidget()
        self.setWidget(content)

        # Create layout and label
        layout = QVBoxLayout(content)
        content.setLayout(layout)
        self.text = QLabel(content)
        self.text.setSizePolicy(QSizePolicy.Preferred, QSizePolicy.Preferred)
        self.text.setFont(CutterBindings.Configuration.instance().getFont())
        layout.addWidget(self.text)

        button = QPushButton(content)
        button.setText("Want a fortune?")
        button.setSizePolicy(QSizePolicy.Maximum, QSizePolicy.Maximum)
        button.setMaximumHeight(50)
        button.setMaximumWidth(200)
        layout.addWidget(button)
        layout.setAlignment(button, Qt.AlignHCenter)

        QObject.connect(CutterBindings.CutterCore.getInstance(), SIGNAL("seekChanged(RVA)"), self.generate_fortune)
        QObject.connect(button, SIGNAL("clicked()"), self.generate_fortune)

        self.show()

    def generate_fortune(self):
        fortune = cutter.cmd("fo").replace("\n", "")
        res = CutterBindings.CutterCore.getInstance().cmdRaw(f"?E {fortune}")
        self.text.setText(res)


class CutterSamplePlugin(CutterPlugin):
    name = "SamplePlugin"
    description = "A sample plugin written in python."
    version = "1.0"
    author = "xarkes and thestr4ng3r :-P"

    def setupInterface(self):
        super().setupInterface()

        self.action = QAction("Sample Python Plugin", self.main)
        self.action.setCheckable(True)
        self.widget = FortuneWidget(self.main, self.action) # we MUST keep a reference to this!
        self.main.addPluginDockWidget(self.widget, self.action)


# Instantiate our plugin
plugin = CutterSamplePlugin()
