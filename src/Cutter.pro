TEMPLATE = app

TARGET = Cutter

# The application version
VERSION = 1.4

ICON = img/cutter.icns

QT += core gui widgets svg
QT_CONFIG -= no-pkg-config
CONFIG += c++11

!defined(CUTTER_ENABLE_JUPYTER, var)        CUTTER_ENABLE_JUPYTER=true
equals(CUTTER_ENABLE_JUPYTER, true)         CONFIG += CUTTER_ENABLE_JUPYTER

!defined(CUTTER_ENABLE_QTWEBENGINE, var)    CUTTER_ENABLE_QTWEBENGINE=true
equals(CUTTER_ENABLE_JUPYTER, true) {
    equals(CUTTER_ENABLE_QTWEBENGINE, true)  CONFIG += CUTTER_ENABLE_QTWEBENGINE
}

!defined(CUTTER_BUNDLE_R2_APPBUNDLE, var)   CUTTER_BUNDLE_R2_APPBUNDLE=false
equals(CUTTER_BUNDLE_R2_APPBUNDLE, true)    CONFIG += CUTTER_BUNDLE_R2_APPBUNDLE

# Define the preprocessor macro to get the application version in our application.
DEFINES += APP_VERSION=\\\"$$VERSION\\\"
CUTTER_ENABLE_JUPYTER {
    message("Jupyter support enabled.")
    DEFINES += CUTTER_ENABLE_JUPYTER
} else {
    message("Jupyter support disabled.")
}

CUTTER_ENABLE_QTWEBENGINE {
    message("QtWebEngine support enabled.")
    DEFINES += CUTTER_ENABLE_QTWEBENGINE
    QT += webenginewidgets
} else {
    message("QtWebEngine support disabled.")
}

INCLUDEPATH *= .

win32 {
    # Generate debug symbols in release mode
    QMAKE_CXXFLAGS_RELEASE += -Zi   # Compiler
    QMAKE_LFLAGS_RELEASE += /DEBUG  # Linker
}

macx {
    QMAKE_CXXFLAGS = -mmacosx-version-min=10.7 -std=gnu0x -stdlib=libc++
    QMAKE_INFO_PLIST = apple/Info.plist
}

unix:exists(/usr/local/include/libr) {
    INCLUDEPATH += /usr/local/include/libr
}

# Libraries
include(lib_radare2.pri)
win32:CUTTER_ENABLE_JUPYTER {
    pythonpath = $$quote($$system("where python"))
    pythonpath = $$replace(pythonpath, ".exe ", ".exe;")
    pythonpath = $$section(pythonpath, ";", 0, 0)
    pythonpath = $$clean_path($$dirname(pythonpath))
    LIBS += -L$${pythonpath} -L$${pythonpath}/libs -lpython3
    INCLUDEPATH += $${pythonpath}/include
}

unix:CUTTER_ENABLE_JUPYTER|macx:CUTTER_ENABLE_JUPYTER {
    defined(PYTHON_FRAMEWORK_DIR, var) {
        message("Using Python.framework at $$PYTHON_FRAMEWORK_DIR")
        INCLUDEPATH += $$PYTHON_FRAMEWORK_DIR/Python.framework/Headers
        LIBS += -F$$PYTHON_FRAMEWORK_DIR -framework Python
        DEFINES += MACOS_PYTHON_FRAMEWORK_BUNDLED
    } else {
        CONFIG += link_pkgconfig
        !packagesExist(python3) {
            error("ERROR: Python 3 could not be found. Make sure it is available to pkg-config.")
        }
        PKGCONFIG += python3
    }
}

macx:CUTTER_BUNDLE_R2_APPBUNDLE {
    message("Using r2 rom AppBundle")
    DEFINES += MACOS_R2_BUNDLED
}

SOURCES += \
    Main.cpp \
    Cutter.cpp \
    widgets/DisassemblerGraphView.cpp \
    utils/RichTextPainter.cpp \
    dialogs/OptionsDialog.cpp \
    dialogs/AboutDialog.cpp \
    dialogs/CommentsDialog.cpp \
    dialogs/EditInstructionDialog.cpp \
    dialogs/FlagDialog.cpp \
    dialogs/RenameDialog.cpp \
    dialogs/XrefsDialog.cpp \
    MainWindow.cpp \
    utils/Helpers.cpp \
    utils/HexAsciiHighlighter.cpp \
    utils/HexHighlighter.cpp \
    utils/Highlighter.cpp \
    utils/MdHighlighter.cpp \
    dialogs/preferences/AsmOptionsWidget.cpp \
    dialogs/NewFileDialog.cpp \
    AnalTask.cpp \
    widgets/CommentsWidget.cpp \
    widgets/ConsoleWidget.cpp \
    widgets/Dashboard.cpp \
    widgets/EntrypointWidget.cpp \
    widgets/ExportsWidget.cpp \
    widgets/FlagsWidget.cpp \
    widgets/FunctionsWidget.cpp \
    widgets/ImportsWidget.cpp \
    widgets/Omnibar.cpp \
    widgets/PieView.cpp \
    widgets/RelocsWidget.cpp \
    widgets/SdbDock.cpp \
    widgets/SectionsWidget.cpp \
    widgets/Sidebar.cpp \
    widgets/StringsWidget.cpp \
    widgets/SymbolsWidget.cpp \
    menus/DisassemblyContextMenu.cpp \
    widgets/DisassemblyWidget.cpp \
    widgets/SidebarWidget.cpp \
    widgets/HexdumpWidget.cpp \
    utils/Configuration.cpp \
    utils/Colors.cpp \
    dialogs/SaveProjectDialog.cpp \
    utils/TempConfig.cpp \
    utils/SvgIconEngine.cpp \
    utils/SyntaxHighlighter.cpp \
    widgets/PseudocodeWidget.cpp \
    widgets/VisualNavbar.cpp \
    widgets/GraphView.cpp \
    dialogs/preferences/PreferencesDialog.cpp \
    dialogs/preferences/GeneralOptionsWidget.cpp \
    dialogs/preferences/GraphOptionsWidget.cpp \
    dialogs/preferences/PreferenceCategory.cpp \
    widgets/QuickFilterView.cpp \
    widgets/ClassesWidget.cpp \
    widgets/ResourcesWidget.cpp \
    widgets/VTablesWidget.cpp \
    widgets/TypesWidget.cpp \
    widgets/HeadersWidget.cpp \
    widgets/SearchWidget.cpp \
    CutterApplication.cpp \
    utils/JupyterConnection.cpp \
    widgets/JupyterWidget.cpp \
    utils/PythonAPI.cpp \
    utils/NestedIPyKernel.cpp \
    dialogs/R2PluginsDialog.cpp \
    widgets/CutterDockWidget.cpp \
    widgets/CutterSeekableWidget.cpp \
    widgets/GraphWidget.cpp \
    utils/JsonTreeItem.cpp \
    utils/JsonModel.cpp \
    dialogs/VersionInfoDialog.cpp \
    widgets/ZignaturesWidget.cpp \
    utils/AsyncTask.cpp \
    dialogs/AsyncTaskDialog.cpp \
    widgets/StackWidget.cpp \
    widgets/RegistersWidget.cpp \
    widgets/BacktraceWidget.cpp \
    dialogs/OpenFileDialog.cpp \
    utils/StringsTask.cpp \
    utils/CommandTask.cpp \
    utils/ProgressIndicator.cpp

HEADERS  += \
    Cutter.h \
    widgets/DisassemblerGraphView.h \
    utils/RichTextPainter.h \
    utils/CachedFontMetrics.h \
    dialogs/AboutDialog.h \
    dialogs/preferences/AsmOptionsWidget.h \
    dialogs/CommentsDialog.h \
    dialogs/EditInstructionDialog.h \
    dialogs/FlagDialog.h \
    dialogs/RenameDialog.h \
    dialogs/XrefsDialog.h \
    utils/Helpers.h \
    utils/HexAsciiHighlighter.h \
    utils/HexHighlighter.h \
    MainWindow.h \
    utils/Highlighter.h \
    utils/MdHighlighter.h \
    dialogs/OptionsDialog.h \
    dialogs/NewFileDialog.h \
    AnalTask.h \
    widgets/CommentsWidget.h \
    widgets/ConsoleWidget.h \
    widgets/Dashboard.h \
    widgets/EntrypointWidget.h \
    widgets/ExportsWidget.h \
    widgets/FlagsWidget.h \
    widgets/FunctionsWidget.h \
    widgets/ImportsWidget.h \
    widgets/Omnibar.h \
    widgets/PieView.h \
    widgets/RelocsWidget.h \
    widgets/SdbDock.h \
    widgets/SectionsWidget.h \
    widgets/Sidebar.h \
    widgets/StringsWidget.h \
    widgets/SymbolsWidget.h \
    menus/DisassemblyContextMenu.h \
    widgets/DisassemblyWidget.h \
    widgets/SidebarWidget.h \
    widgets/HexdumpWidget.h \
    utils/Configuration.h \
    utils/Colors.h \
    dialogs/SaveProjectDialog.h \
    utils/TempConfig.h \
    utils/SvgIconEngine.h \
    utils/SyntaxHighlighter.h \
    widgets/PseudocodeWidget.h \
    widgets/VisualNavbar.h \
    widgets/GraphView.h \
    dialogs/preferences/PreferencesDialog.h \
    dialogs/preferences/GeneralOptionsWidget.h \
    dialogs/preferences/PreferenceCategory.h \
    dialogs/preferences/GraphOptionsWidget.h \
    widgets/QuickFilterView.h \
    widgets/ClassesWidget.h \
    widgets/ResourcesWidget.h \
    CutterApplication.h \
    widgets/VTablesWidget.h \
    widgets/TypesWidget.h \
    widgets/HeadersWidget.h \
    widgets/SearchWidget.h \
    utils/JupyterConnection.h \
    widgets/JupyterWidget.h \
    utils/PythonAPI.h \
    utils/NestedIPyKernel.h \
    dialogs/R2PluginsDialog.h \
    widgets/CutterDockWidget.h \
    widgets/CutterSeekableWidget.h \
    widgets/GraphWidget.h \
    utils/JsonTreeItem.h \
    utils/JsonModel.h \
    dialogs/VersionInfoDialog.h \
    widgets/ZignaturesWidget.h \
    utils/AsyncTask.h \
    dialogs/AsyncTaskDialog.h \
    widgets/StackWidget.h \
    widgets/RegistersWidget.h \
    widgets/BacktraceWidget.h \
    dialogs/OpenFileDialog.h \
    utils/StringsTask.h \
    utils/CommandTask.h \
    utils/ProgressIndicator.h

FORMS    += \
    dialogs/AboutDialog.ui \
    dialogs/preferences/AsmOptionsWidget.ui \
    dialogs/CommentsDialog.ui \
    dialogs/EditInstructionDialog.ui \
    dialogs/FlagDialog.ui \
    dialogs/RenameDialog.ui \
    dialogs/XrefsDialog.ui \
    dialogs/NewfileDialog.ui \
    dialogs/OptionsDialog.ui \
    MainWindow.ui \
    widgets/CommentsWidget.ui \
    widgets/ConsoleWidget.ui \
    widgets/Dashboard.ui \
    widgets/EntrypointWidget.ui \
    widgets/FlagsWidget.ui \
    widgets/ExportsWidget.ui \
    widgets/FunctionsWidget.ui \
    widgets/ImportsWidget.ui \
    widgets/SdbDock.ui \
    widgets/RelocsWidget.ui \
    widgets/SectionsWidget.ui \
    widgets/Sidebar.ui \
    widgets/StringsWidget.ui \
    widgets/SymbolsWidget.ui \
    widgets/SidebarWidget.ui \
    widgets/HexdumpWidget.ui \
    dialogs/SaveProjectDialog.ui \
    dialogs/preferences/PreferencesDialog.ui \
    dialogs/preferences/GeneralOptionsWidget.ui \
    dialogs/preferences/GraphOptionsWidget.ui \
    widgets/QuickFilterView.ui \
    widgets/PseudocodeWidget.ui \
    widgets/ClassesWidget.ui \
    widgets/VTablesWidget.ui \
    widgets/TypesWidget.ui \
    widgets/HeadersWidget.ui \
    widgets/SearchWidget.ui \
    widgets/JupyterWidget.ui \
    dialogs/R2PluginsDialog.ui \
    dialogs/VersionInfoDialog.ui \
    widgets/ZignaturesWidget.ui \
    dialogs/AsyncTaskDialog.ui \
    widgets/StackWidget.ui \
    widgets/RegistersWidget.ui \
    widgets/BacktraceWidget.ui \
    dialogs/OpenFileDialog.ui

RESOURCES += \
    resources.qrc \
    themes/qdarkstyle/style.qrc


DISTFILES += Cutter.astylerc

# 'make install' for AppImage
unix {
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }

    icon_file = img/cutter.svg

    share_pixmaps.path = $$PREFIX/share/pixmaps
    share_pixmaps.files = $$icon_file


    desktop_file = Cutter.desktop

    share_applications.path = $$PREFIX/share/applications
    share_applications.files = $$desktop_file

    appstream_file = Cutter.appdata.xml

    share_appdata.path = $$PREFIX/share/appdata
    share_appdata.files = $$appstream_file

    # built-in no need for files atm
    target.path = $$PREFIX/bin

    INSTALLS += target share_appdata share_applications share_pixmaps

    # Triggered for example by 'qmake APPIMAGE=1'
    !isEmpty(APPIMAGE){
        appimage_root.path = /
        appimage_root.files = $$icon_file $$desktop_file

        INSTALLS += appimage_root
        DEFINES += APPIMAGE
    }
}
