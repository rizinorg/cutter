
import cutter
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

        QObject.connect(cutter.core(), SIGNAL("seekChanged(RVA)"), self.generate_fortune)
        QObject.connect(button, SIGNAL("clicked()"), self.generate_fortune)

        self.show()

    def generate_fortune(self):
        fortune = cutter.cmd("fo").replace("\n", "")
        res = cutter.core().cmdRaw(f"?E {fortune}")
        self.text.setText(res)


class CutterSamplePlugin(CutterBindings.CutterPlugin):
    name = "SamplePlugin"
    description = "A sample plugin written in python."
    version = "1.0"
    author = "xarkes and thestr4ng3r :-P"

    def __init__(self):
        super(CutterSamplePlugin, self).__init__()

    def setupPlugin(self):
        pass

    def setupInterface(self, main):
        self.action = QAction("Sample Python Plugin", main)
        self.action.setCheckable(True)
        self.widget = FortuneWidget(main, self.action) # we MUST keep a reference to this!
        main.addPluginDockWidget(self.widget, self.action)


# This function will be called by Cutter and should return an instance of the plugin.
def create_cutter_plugin():
    return CutterSamplePlugin()
