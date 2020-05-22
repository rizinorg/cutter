
import cutter

from PySide2.QtCore import Qt
from PySide2.QtWidgets import QVBoxLayout, QLabel, QWidget, QSizePolicy, QPushButton


class FortuneWidget(cutter.CutterDockWidget):
    def __init__(self, parent):
        super(FortuneWidget, self).__init__(parent)
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
    name = "Sample Plugin"
    description = "A sample plugin written in python."
    version = "1.1"
    author = "Cutter developers"

    # Override CutterPlugin methods

    def __init__(self):
        super(CutterSamplePlugin, self).__init__()
        self.disassembly_actions = []
        self.addressable_item_actions = []
        self.disas_action = None
        self.addr_submenu = None
        self.main = None

    def setupPlugin(self):
        pass

    def setupInterface(self, main):
        # Dock widget
        widget = FortuneWidget(main)
        main.addPluginDockWidget(widget)

        # Dissassembly context menu
        menu = main.getContextMenuExtensions(cutter.MainWindow.ContextMenuType.Disassembly)
        self.disas_action = menu.addAction("CutterSamplePlugin dissassembly action")
        self.disas_action.triggered.connect(self.handle_disassembler_action)
        self.main = main

        # Context menu for tables with addressable items like Flags,Functions,Strings,Search results,...
        addressable_item_menu = main.getContextMenuExtensions(cutter.MainWindow.ContextMenuType.Addressable)
        self.addr_submenu = addressable_item_menu.addMenu("CutterSamplePlugin") # create submenu
        adrr_action = self.addr_submenu.addAction("Action 1")
        self.addr_submenu.addSeparator() # can use separator and other qt functionality
        adrr_action2 = self.addr_submenu.addAction("Action 2")
        adrr_action.triggered.connect(self.handle_addressable_item_action)
        adrr_action2.triggered.connect(self.handle_addressable_item_action)

    def terminate(self): # optional
        print("CutterSamplePlugin shutting down")
        if self.main:
            menu = self.main.getContextMenuExtensions(cutter.MainWindow.ContextMenuType.Disassembly)
            menu.removeAction(self.disas_action)
            addressable_item_menu = self.main.getContextMenuExtensions(cutter.MainWindow.ContextMenuType.Addressable)
            submenu_action = self.addr_submenu.menuAction()
            addressable_item_menu.removeAction(submenu_action)
        print("CutterSamplePlugin finished clean up")

    # Plugin methods

    def handle_addressable_item_action(self):
        # for actions in plugin menu Cutter sets data to current item address
        submenu_action = self.addr_submenu.menuAction()
        cutter.message("Context menu action callback 0x{:x}".format(submenu_action.data()))

    def handle_disassembler_action(self):
        # for actions in plugin menu Cutter sets data to address for current dissasembly line
        cutter.message("Dissasembly menu action callback 0x{:x}".format(self.disas_action.data()))


# This function will be called by Cutter and should return an instance of the plugin.
def create_cutter_plugin():
    return CutterSamplePlugin()
