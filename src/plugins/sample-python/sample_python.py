
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
    version = "1.1"
    author = "xarkes and thestr4ng3r :-P"

    # Override CutterPlugin methods

    def __init__(self):
        super(CutterSamplePlugin, self).__init__()
        self.disassembly_actions = []
        self.addressable_item_actions = []
        self.testDisasAction = None
        self.testAddrAction = None
        self.testAddrAction2 = None
        self.main = None

    def setupPlugin(self):
        pass

    def setupInterface(self, main):
        # Dock widget
        action = QAction("Sample Python Plugin", main)
        action.setCheckable(True)
        widget = FortuneWidget(main, action)
        main.addPluginDockWidget(widget, action)

        # Dissassembly context menu
        menu = main.getContextMenuExtensions(cutter.MainWindow.ContextMenuType.Disassembly)
        self.testDisasAction = menu.addAction("Test disassembly")
        self.testDisasAction.triggered.connect(self.handleDisas)
        self.main = main

        # Context menu for tables with addressable items like Flags,Functions,Strings,Search results,...
        addressableItemMenu = main.getContextMenuExtensions(cutter.MainWindow.ContextMenuType.Addressable)
        self.testAddrAction = addressableItemMenu.addAction("Test addressable")
        self.addrSeparator = addressableItemMenu.addSeparator() # can use separator and other qt functionality
        self.testAddrAction2 = addressableItemMenu.addAction("Test addressable2")
        self.testAddrAction.triggered.connect(self.handleAddr)
        self.testAddrAction2.triggered.connect(self.handleAddr)

    def terminate(self): # optional
        print("CutterSamplePlugin shutting down")
        if self.main:
            menu = self.main.getContextMenuExtensions(cutter.MainWindow.ContextMenuType.Disassembly)
            menu.removeAction(self.testDisasAction)
            addressableItemMenu = self.main.getContextMenuExtensions(cutter.MainWindow.ContextMenuType.Addressable)
            for action in [self.testAddrAction, self.testAddrAction2, self.addrSeparator]:
                addressableItemMenu.removeAction(action)
        print("CutterSamplePlugin finished clean up")

    # Plugin methods

    def handleAddr(self):
        # for actions in plugin menu Cutter sets data to current item address
        cutter.message("Context menu action callback 0x{:x}".format(self.testAddrAction.data()))

    def handleDisas(self):
        # for actions in plugin menu Cutter sets data to address for current dissasembly line
        cutter.message("Dissasembly menu action callback 0x{:x}".format(self.testDisasAction.data()))


# This function will be called by Cutter and should return an instance of the plugin.
def create_cutter_plugin():
    return CutterSamplePlugin()
