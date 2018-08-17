TEMPLATE = app

TARGET = Cutter

CUTTER_VERSION_MAJOR = 1
CUTTER_VERSION_MINOR = 7
CUTTER_VERSION_PATCH = 4

VERSION = $${CUTTER_VERSION_MAJOR}.$${CUTTER_VERSION_MINOR}.$${CUTTER_VERSION_PATCH}

# Required QT version
lessThan(QT_MAJOR_VERSION, 5): error("requires Qt 5")

TRANSLATIONS += translations/cutter_ca.ts \
                translations/cutter_de.ts \
                translations/cutter_es.ts \
                translations/cutter_fr.ts \
                translations/cutter_it.ts \
                translations/cutter_nl.ts \
                translations/cutter_pt.ts \
                translations/cutter_ro.ts \
                translations/cutter_ru.ts \
                translations/cutter_tr.ts

# Icon for OS X
ICON = img/cutter.icns

# Icon/resources for Windows
win32: RC_ICONS = img/cutter.ico

QT += core gui widgets svg network
QT_CONFIG -= no-pkg-config
CONFIG += c++11

!defined(CUTTER_ENABLE_JUPYTER, var)        CUTTER_ENABLE_JUPYTER=true
equals(CUTTER_ENABLE_JUPYTER, true)         CONFIG += CUTTER_ENABLE_JUPYTER

!defined(CUTTER_ENABLE_QTWEBENGINE, var)    CUTTER_ENABLE_QTWEBENGINE=false
equals(CUTTER_ENABLE_JUPYTER, true) {
    equals(CUTTER_ENABLE_QTWEBENGINE, true)  CONFIG += CUTTER_ENABLE_QTWEBENGINE
}

!defined(CUTTER_BUNDLE_R2_APPBUNDLE, var)   CUTTER_BUNDLE_R2_APPBUNDLE=false
equals(CUTTER_BUNDLE_R2_APPBUNDLE, true)    CONFIG += CUTTER_BUNDLE_R2_APPBUNDLE

!defined(CUTTER_APPVEYOR_R2DEC, var)        CUTTER_APPVEYOR_R2DEC=false
equals(CUTTER_APPVEYOR_R2DEC, true)         CONFIG += CUTTER_APPVEYOR_R2DEC

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
	
    # Multithreaded compilation
    QMAKE_CXXFLAGS += -MP
}

macx {
    QMAKE_CXXFLAGS = -mmacosx-version-min=10.7 -std=gnu0x -stdlib=libc++
    QMAKE_TARGET_BUNDLE_PREFIX = org.radare
    QMAKE_BUNDLE = cutter
    QMAKE_INFO_PLIST = macos/Info.plist
}

unix:exists(/usr/local/include/libr)|bsd:exists(/usr/local/include/libr) {
    INCLUDEPATH += /usr/local/include/libr
}
unix {
    QMAKE_LFLAGS += -rdynamic # Export dynamic symbols for plugins
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

unix:CUTTER_ENABLE_JUPYTER|macx:CUTTER_ENABLE_JUPYTER|bsd:CUTTER_ENABLE_JUPYTER {
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

CUTTER_APPVEYOR_R2DEC {
    message("Appveyor r2dec")
    DEFINES += CUTTER_APPVEYOR_R2DEC
}

QMAKE_SUBSTITUTES += CutterConfig.h.in

SOURCES += \
    Main.cpp \
    Cutter.cpp \
    widgets/DisassemblerGraphView.cpp \
    widgets/OverviewView.cpp \
    common/RichTextPainter.cpp \
    dialogs/InitialOptionsDialog.cpp \
    dialogs/AboutDialog.cpp \
    dialogs/CommentsDialog.cpp \
    dialogs/EditInstructionDialog.cpp \
    dialogs/FlagDialog.cpp \
    dialogs/RenameDialog.cpp \
    dialogs/XrefsDialog.cpp \
    MainWindow.cpp \
    common/Helpers.cpp \
    common/HexAsciiHighlighter.cpp \
    common/HexHighlighter.cpp \
    common/Highlighter.cpp \
    common/MdHighlighter.cpp \
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
    widgets/RelocsWidget.cpp \
    widgets/SdbDock.cpp \
    widgets/SectionsWidget.cpp \
    widgets/SegmentsWidget.cpp \
    widgets/StringsWidget.cpp \
    widgets/SymbolsWidget.cpp \
    menus/DisassemblyContextMenu.cpp \
    widgets/DisassemblyWidget.cpp \
    widgets/HexdumpWidget.cpp \
    common/Configuration.cpp \
    common/Colors.cpp \
    dialogs/SaveProjectDialog.cpp \
    common/TempConfig.cpp \
    common/SvgIconEngine.cpp \
    common/SyntaxHighlighter.cpp \
    widgets/PseudocodeWidget.cpp \
    widgets/VisualNavbar.cpp \
    widgets/GraphView.cpp \
    dialogs/preferences/PreferencesDialog.cpp \
    dialogs/preferences/AppearanceOptionsWidget.cpp \
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
    common/JupyterConnection.cpp \
    widgets/JupyterWidget.cpp \
    common/PythonAPI.cpp \
    common/NestedIPyKernel.cpp \
    dialogs/R2PluginsDialog.cpp \
    widgets/CutterDockWidget.cpp \
    widgets/CutterTreeWidget.cpp \
    widgets/GraphWidget.cpp \
    widgets/OverviewWidget.cpp \
    common/JsonTreeItem.cpp \
    common/JsonModel.cpp \
    dialogs/VersionInfoDialog.cpp \
    widgets/ZignaturesWidget.cpp \
    common/AsyncTask.cpp \
    dialogs/AsyncTaskDialog.cpp \
    widgets/StackWidget.cpp \
    widgets/RegistersWidget.cpp \
    widgets/BacktraceWidget.cpp \
    dialogs/OpenFileDialog.cpp \
    common/CommandTask.cpp \
    common/ProgressIndicator.cpp \
    common/R2Task.cpp \
    widgets/DebugActions.cpp \
    widgets/MemoryMapWidget.cpp \
    dialogs/preferences/DebugOptionsWidget.cpp \
    widgets/BreakpointWidget.cpp \
    dialogs/BreakpointsDialog.cpp \
    dialogs/AttachProcDialog.cpp \
    widgets/RegisterRefsWidget.cpp \
    dialogs/SetToDataDialog.cpp \
    dialogs/EditVariablesDialog.cpp \
    widgets/ColorSchemePrefWidget.cpp \
    common/ColorSchemeFileSaver.cpp \
    dialogs/EditFunctionDialog.cpp \
    widgets/CutterTreeView.cpp \
    widgets/ComboQuickFilterView.cpp \
    dialogs/HexdumpRangeDialog.cpp \
    common/QtResImporter.cpp \
    common/CutterSeekable.cpp \
    common/RefreshDeferrer.cpp \
    dialogs/WelcomeDialog.cpp \
    RunScriptTask.cpp \
    dialogs/EditMethodDialog.cpp

HEADERS  += \
    Cutter.h \
    widgets/DisassemblerGraphView.h \
    widgets/OverviewView.h \
    common/RichTextPainter.h \
    common/CachedFontMetrics.h \
    dialogs/AboutDialog.h \
    dialogs/preferences/AsmOptionsWidget.h \
    dialogs/CommentsDialog.h \
    dialogs/EditInstructionDialog.h \
    dialogs/FlagDialog.h \
    dialogs/RenameDialog.h \
    dialogs/XrefsDialog.h \
    common/Helpers.h \
    common/HexAsciiHighlighter.h \
    common/HexHighlighter.h \
    MainWindow.h \
    common/Highlighter.h \
    common/MdHighlighter.h \
    dialogs/InitialOptionsDialog.h \
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
    widgets/RelocsWidget.h \
    widgets/SdbDock.h \
    widgets/SectionsWidget.h \
    widgets/SegmentsWidget.h \
    widgets/StringsWidget.h \
    widgets/SymbolsWidget.h \
    menus/DisassemblyContextMenu.h \
    widgets/DisassemblyWidget.h \
    widgets/HexdumpWidget.h \
    common/Configuration.h \
    common/Colors.h \
    dialogs/SaveProjectDialog.h \
    common/TempConfig.h \
    common/SvgIconEngine.h \
    common/SyntaxHighlighter.h \
    widgets/PseudocodeWidget.h \
    widgets/VisualNavbar.h \
    widgets/GraphView.h \
    dialogs/preferences/PreferencesDialog.h \
    dialogs/preferences/AppearanceOptionsWidget.h \
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
    common/JupyterConnection.h \
    widgets/JupyterWidget.h \
    common/PythonAPI.h \
    common/NestedIPyKernel.h \
    dialogs/R2PluginsDialog.h \
    widgets/CutterDockWidget.h \
    widgets/CutterTreeWidget.h \
    widgets/GraphWidget.h \
    widgets/OverviewWidget.h \
    common/JsonTreeItem.h \
    common/JsonModel.h \
    dialogs/VersionInfoDialog.h \
    widgets/ZignaturesWidget.h \
    common/AsyncTask.h \
    dialogs/AsyncTaskDialog.h \
    widgets/StackWidget.h \
    widgets/RegistersWidget.h \
    widgets/BacktraceWidget.h \
    dialogs/OpenFileDialog.h \
    common/StringsTask.h \
    common/FunctionsTask.h \
    common/CommandTask.h \
    common/ProgressIndicator.h \
    plugins/CutterPlugin.h \
    common/R2Task.h \
    widgets/DebugActions.h \
    widgets/MemoryMapWidget.h \
    dialogs/preferences/DebugOptionsWidget.h \
    widgets/BreakpointWidget.h \
    dialogs/BreakpointsDialog.h \
    dialogs/AttachProcDialog.h \
    widgets/RegisterRefsWidget.h \
    dialogs/SetToDataDialog.h \
    common/InitialOptions.h \
    dialogs/EditVariablesDialog.h \
    common/ColorSchemeFileSaver.h \
    widgets/ColorSchemePrefWidget.h \
    dialogs/EditFunctionDialog.h \
    widgets/CutterTreeView.h \
    widgets/ComboQuickFilterView.h \
    dialogs/HexdumpRangeDialog.h \
    common/QtResImporter.h \
    common/CutterSeekable.h \
    common/RefreshDeferrer.h \
    dialogs/WelcomeDialog.h \
    RunScriptTask.h \
    common/Json.h \
    dialogs/EditMethodDialog.h

FORMS    += \
    dialogs/AboutDialog.ui \
    dialogs/preferences/AsmOptionsWidget.ui \
    dialogs/CommentsDialog.ui \
    dialogs/EditInstructionDialog.ui \
    dialogs/FlagDialog.ui \
    dialogs/RenameDialog.ui \
    dialogs/XrefsDialog.ui \
    dialogs/NewfileDialog.ui \
    dialogs/InitialOptionsDialog.ui \
    dialogs/EditFunctionDialog.ui \
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
    widgets/StringsWidget.ui \
    widgets/SymbolsWidget.ui \
    widgets/HexdumpWidget.ui \
    dialogs/SaveProjectDialog.ui \
    dialogs/preferences/PreferencesDialog.ui \
    dialogs/preferences/AppearanceOptionsWidget.ui \
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
    dialogs/OpenFileDialog.ui \
    widgets/MemoryMapWidget.ui \
    dialogs/preferences/DebugOptionsWidget.ui \
    widgets/BreakpointWidget.ui \
    dialogs/BreakpointsDialog.ui \
    dialogs/AttachProcDialog.ui \
    widgets/RegisterRefsWidget.ui \
    dialogs/SetToDataDialog.ui \
    dialogs/EditVariablesDialog.ui \
    widgets/ColorSchemePrefWidget.ui \
    widgets/CutterTreeView.ui \
    widgets/ComboQuickFilterView.ui \
    dialogs/HexdumpRangeDialog.ui \
    dialogs/WelcomeDialog.ui \
    dialogs/EditMethodDialog.ui

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


    desktop_file = org.radare.Cutter.desktop

    share_applications.path = $$PREFIX/share/applications
    share_applications.files = $$desktop_file

    appstream_file = org.radare.Cutter.appdata.xml

    # Used by ???
    share_appdata.path = $$PREFIX/share/appdata
    share_appdata.files = $$appstream_file

    # Used by AppImageHub (See https://www.freedesktop.org/software/appstream)
    share_appdata.path = $$PREFIX/share/metainfo
    share_appdata.files = $$appstream_file

    # built-in no need for files atm
    target.path = $$PREFIX/bin

    INSTALLS += target share_appdata share_metadata share_applications share_pixmaps

    # Triggered for example by 'qmake APPIMAGE=1'
    !isEmpty(APPIMAGE){
        appimage_root.path = /
        appimage_root.files = $$icon_file $$desktop_file

        INSTALLS += appimage_root
        DEFINES += APPIMAGE
    }
}
