TEMPLATE = app

TARGET = Cutter

CUTTER_VERSION_MAJOR = 1
CUTTER_VERSION_MINOR = 10
CUTTER_VERSION_PATCH = 3

VERSION = $${CUTTER_VERSION_MAJOR}.$${CUTTER_VERSION_MINOR}.$${CUTTER_VERSION_PATCH}

# Required QT version
lessThan(QT_MAJOR_VERSION, 5): error("requires Qt 5")

TRANSLATIONS += translations/cutter_ar.ts \
                translations/cutter_ca.ts \
                translations/cutter_cn.ts \
                translations/cutter_de.ts \
                translations/cutter_es.ts \
                translations/cutter_fa.ts \
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

!defined(CUTTER_ENABLE_CRASH_REPORTS, var)      CUTTER_ENABLE_CRASH_REPORTS=false
equals(CUTTER_ENABLE_CRASH_REPORTS, true)       CONFIG += CUTTER_ENABLE_CRASH_REPORTS

!defined(CUTTER_ENABLE_PYTHON, var)             CUTTER_ENABLE_PYTHON=false
equals(CUTTER_ENABLE_PYTHON, true)              CONFIG += CUTTER_ENABLE_PYTHON

!defined(CUTTER_ENABLE_PYTHON_BINDINGS, var)    CUTTER_ENABLE_PYTHON_BINDINGS=false
equals(CUTTER_ENABLE_PYTHON, true) {
    equals(CUTTER_ENABLE_PYTHON_BINDINGS, true) {
        CONFIG += CUTTER_ENABLE_PYTHON_BINDINGS
    }
}

!defined(CUTTER_BUNDLE_R2_APPBUNDLE, var)       CUTTER_BUNDLE_R2_APPBUNDLE=false
equals(CUTTER_BUNDLE_R2_APPBUNDLE, true)        CONFIG += CUTTER_BUNDLE_R2_APPBUNDLE

!defined(CUTTER_APPVEYOR_R2DEC, var)            CUTTER_APPVEYOR_R2DEC=false
equals(CUTTER_APPVEYOR_R2DEC, true)             CONFIG += CUTTER_APPVEYOR_R2DEC

!defined(CUTTER_R2GHIDRA_STATIC, var)           CUTTER_R2GHIDRA_STATIC=false
equals(CUTTER_R2GHIDRA_STATIC, true)            CONFIG += CUTTER_R2GHIDRA_STATIC

CUTTER_ENABLE_CRASH_REPORTS {
    message("Crash report support enabled.")
    DEFINES += CUTTER_ENABLE_CRASH_REPORTS
} else {
    message("Crash report support disabled.")
}

CUTTER_ENABLE_PYTHON {
    message("Python enabled.")
    DEFINES += CUTTER_ENABLE_PYTHON
} else {
    message("Python disabled.")
}

CUTTER_ENABLE_PYTHON_BINDINGS {
    message("Python Bindings enabled.")
    DEFINES += CUTTER_ENABLE_PYTHON_BINDINGS
} else {
    message("Python Bindings disabled. (requires CUTTER_ENABLE_PYTHON=true)")
}

win32:defined(CUTTER_DEPS_DIR, var) {
    !defined(SHIBOKEN_EXECUTABLE, var)          SHIBOKEN_EXECUTABLE="$${CUTTER_DEPS_DIR}/pyside/bin/shiboken2.exe"
    !defined(SHIBOKEN_INCLUDEDIR, var)          SHIBOKEN_INCLUDEDIR="$${CUTTER_DEPS_DIR}/pyside/include/shiboken2"
    !defined(SHIBOKEN_LIBRARY, var)             SHIBOKEN_LIBRARY="$${CUTTER_DEPS_DIR}/pyside/lib/shiboken2.abi3.lib"
    !defined(PYSIDE_INCLUDEDIR, var)            PYSIDE_INCLUDEDIR="$${CUTTER_DEPS_DIR}/pyside/include/PySide2"
    !defined(PYSIDE_LIBRARY, var)               PYSIDE_LIBRARY="$${CUTTER_DEPS_DIR}/pyside/lib/pyside2.abi3.lib"
    !defined(PYSIDE_TYPESYSTEMS, var)           PYSIDE_TYPESYSTEMS="$${CUTTER_DEPS_DIR}/pyside/share/PySide2/typesystems"
}

INCLUDEPATH *= . core widgets dialogs common plugins menus

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

!win32 {
    CONFIG += link_pkgconfig
}

CUTTER_ENABLE_PYTHON {
    win32 {
        PYTHON_EXECUTABLE = $$system("where python", lines)
        PYTHON_EXECUTABLE = $$first(PYTHON_EXECUTABLE)
        pythonpath = $$clean_path($$dirname(PYTHON_EXECUTABLE))
        LIBS += -L$${pythonpath}/libs -lpython3
        INCLUDEPATH += $${pythonpath}/include
    }

    unix|macx|bsd {
        defined(PYTHON_FRAMEWORK_DIR, var) {
            message("Using Python.framework at $$PYTHON_FRAMEWORK_DIR")
            INCLUDEPATH += $$PYTHON_FRAMEWORK_DIR/Python.framework/Headers
            LIBS += -F$$PYTHON_FRAMEWORK_DIR -framework Python
            DEFINES += MACOS_PYTHON_FRAMEWORK_BUNDLED
        } else {
            !packagesExist(python3) {
                error("ERROR: Python 3 could not be found. Make sure it is available to pkg-config.")
            }
            PKGCONFIG += python3
        }
    }

    CUTTER_ENABLE_PYTHON_BINDINGS {
        isEmpty(SHIBOKEN_EXECUTABLE):!packagesExist(shiboken2) {
            error("ERROR: Shiboken2, which is required to build the Python Bindings, could not be found. Make sure it is available to pkg-config.")
        }
        isEmpty(PYSIDE_LIBRARY):!packagesExist(pyside2) {
            error("ERROR: PySide2, which is required to build the Python Bindings, could not be found. Make sure it is available to pkg-config.")
        }
        win32 {
            BINDINGS_SRC_LIST_CMD = "\"$${PYTHON_EXECUTABLE}\" bindings/src_list.py"
        } else {
            BINDINGS_SRC_LIST_CMD = "python3 bindings/src_list.py"
        }
        BINDINGS_SRC_DIR = "$${PWD}/bindings"
        BINDINGS_BUILD_DIR = "$${OUT_PWD}/bindings"
        BINDINGS_SOURCE_DIR = "$${BINDINGS_BUILD_DIR}/CutterBindings"
        BINDINGS_SOURCE = $$system("$${BINDINGS_SRC_LIST_CMD} qmake \"$${BINDINGS_BUILD_DIR}\"")
        BINDINGS_INCLUDE_DIRS = "$$[QT_INSTALL_HEADERS]" \
                                "$$[QT_INSTALL_HEADERS]/QtCore" \
                                "$$[QT_INSTALL_HEADERS]/QtWidgets" \
                                "$$[QT_INSTALL_HEADERS]/QtGui" \
                                "$$R2_INCLUDEPATH"
        for(path, INCLUDEPATH) {
            BINDINGS_INCLUDE_DIRS += $$absolute_path("$$path")
        }

        win32 {
            PATH_SEP = ";"
        } else {
            PATH_SEP = ":"
        }
        BINDINGS_INCLUDE_DIRS = $$join(BINDINGS_INCLUDE_DIRS, $$PATH_SEP)

        isEmpty(SHIBOKEN_EXECUTABLE) {
            SHIBOKEN_EXECUTABLE = $$system("pkg-config --variable=generator_location shiboken2")
        }

        isEmpty(PYSIDE_TYPESYSTEMS) {
            PYSIDE_TYPESYSTEMS = $$system("pkg-config --variable=typesystemdir pyside2")
        }
        isEmpty(PYSIDE_INCLUDEDIR) {
            PYSIDE_INCLUDEDIR = $$system("pkg-config --variable=includedir pyside2")
        }

        QMAKE_SUBSTITUTES += bindings/bindings.txt.in

        SHIBOKEN_OPTIONS = --project-file="$${BINDINGS_BUILD_DIR}/bindings.txt"
        defined(SHIBOKEN_EXTRA_OPTIONS, var) SHIBOKEN_OPTIONS += $${SHIBOKEN_EXTRA_OPTIONS}

        win32:SHIBOKEN_OPTIONS += --avoid-protected-hack
        bindings.target = bindings_target
        bindings.commands = $$quote($$system_path($${SHIBOKEN_EXECUTABLE})) $${SHIBOKEN_OPTIONS}
        QMAKE_EXTRA_TARGETS += bindings
        PRE_TARGETDEPS += bindings_target
        # GENERATED_SOURCES += $${BINDINGS_SOURCE} done by dummy targets bellow

        INCLUDEPATH += "$${BINDINGS_SOURCE_DIR}"

        win32:DEFINES += WIN32_LEAN_AND_MEAN

        !isEmpty(PYSIDE_LIBRARY) {
            LIBS += "$$SHIBOKEN_LIBRARY" "$$PYSIDE_LIBRARY"
            INCLUDEPATH += "$$SHIBOKEN_INCLUDEDIR"
        } else:macx {
            # Hack needed because with regular PKGCONFIG qmake will mess up everything
            QMAKE_CXXFLAGS += $$system("pkg-config --cflags shiboken2 pyside2")
            LIBS += $$system("pkg-config --libs shiboken2 pyside2")
        } else {
            PKGCONFIG += shiboken2 pyside2
        }
        INCLUDEPATH += "$$PYSIDE_INCLUDEDIR" "$$PYSIDE_INCLUDEDIR/QtCore" "$$PYSIDE_INCLUDEDIR/QtWidgets" "$$PYSIDE_INCLUDEDIR/QtGui"


        BINDINGS_DUMMY_INPUT_LIST = bindings/src_list.py

        # dummy rules to specify dependency between generated binding files and bindings_target
        bindings_h.input = BINDINGS_DUMMY_INPUT_LIST
        bindings_h.depends = bindings_target
        bindings_h.output = cutterbindings_python.h
        bindings_h.commands = "echo placeholder command ${QMAKE_FILE_OUT}"
        bindings_h.variable_out = HEADERS
        QMAKE_EXTRA_COMPILERS += bindings_h

        for(path, BINDINGS_SOURCE) {
            dummy_input = $$replace(path, .cpp, .txt)
            BINDINGS_DUMMY_INPUTS += $$dummy_input
            win32 {
                _ = $$system("mkdir \"$$dirname(dummy_input)\"; echo a >\"$$dummy_input\"")
            } else {
                _ = $$system("mkdir -p \"$$dirname(dummy_input)\"; echo a >\"$$dummy_input\"")
            }
        }

        bindings_cpp.input = BINDINGS_DUMMY_INPUTS
        bindings_cpp.depends = bindings_target
        bindings_cpp.output = "$${BINDINGS_SOURCE_DIR}/${QMAKE_FILE_IN_BASE}.cpp"
        bindings_cpp.commands = "echo placeholder command ${QMAKE_FILE_OUT}"
        bindings_cpp.variable_out = GENERATED_SOURCES
        QMAKE_EXTRA_COMPILERS += bindings_cpp
    }
}

CUTTER_ENABLE_CRASH_REPORTS {
QMAKE_CXXFLAGS += -g
    defined(BREAKPAD_FRAMEWORK_DIR, var)|defined(BREAKPAD_SOURCE_DIR, var) {
        defined(BREAKPAD_FRAMEWORK_DIR, var) {
            INCLUDEPATH += $$BREAKPAD_FRAMEWORK_DIR/Breakpad.framework/Headers
            LIBS += -F$$BREAKPAD_FRAMEWORK_DIR -framework Breakpad
        }
        defined(BREAKPAD_SOURCE_DIR, var) {
            INCLUDEPATH += $$BREAKPAD_SOURCE_DIR
            win32 {
                LIBS += -L$$quote($$BREAKPAD_SOURCE_DIR\\client\\windows\\release\\lib) -lexception_handler -lcrash_report_sender -lcrash_generation_server -lcrash_generation_client -lcommon
            }
            unix:LIBS += -L$$BREAKPAD_SOURCE_DIR/client/linux -lbreakpad-client
            macos:error("Please use scripts\prepare_breakpad_macos.sh script to provide breakpad framework.")
        }
    } else {
        CONFIG += link_pkgconfig
        !packagesExist(breakpad-client) {
            error("ERROR: Breakpad could not be found. Make sure it is available to pkg-config.")
        }
        PKGCONFIG += breakpad-client
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

CUTTER_R2GHIDRA_STATIC {
    message("Building with static r2ghidra support")
    DEFINES += CUTTER_R2GHIDRA_STATIC
    SOURCES += $$R2GHIDRA_SOURCE/cutter-plugin/R2GhidraDecompiler.cpp
    HEADERS += $$R2GHIDRA_SOURCE/cutter-plugin/R2GhidraDecompiler.h
    INCLUDEPATH += $$R2GHIDRA_SOURCE/cutter-plugin
}

QMAKE_SUBSTITUTES += CutterConfig.h.in

SOURCES += \
    Main.cpp \
    core/Cutter.cpp \
    dialogs/EditStringDialog.cpp \
    dialogs/WriteCommandsDialogs.cpp \
    widgets/DisassemblerGraphView.cpp \
    widgets/OverviewView.cpp \
    common/RichTextPainter.cpp \
    dialogs/InitialOptionsDialog.cpp \
    dialogs/AboutDialog.cpp \
    dialogs/CommentsDialog.cpp \
    dialogs/EditInstructionDialog.cpp \
    dialogs/FlagDialog.cpp \
    dialogs/RenameDialog.cpp \
    dialogs/RemoteDebugDialog.cpp \
    dialogs/NativeDebugDialog.cpp \
    dialogs/XrefsDialog.cpp \
    core/MainWindow.cpp \
    common/Helpers.cpp \
    common/HexAsciiHighlighter.cpp \
    common/HexHighlighter.cpp \
    common/Highlighter.cpp \
    common/MdHighlighter.cpp \
    common/DirectionalComboBox.cpp \
    dialogs/preferences/AsmOptionsWidget.cpp \
    dialogs/NewFileDialog.cpp \
    common/AnalTask.cpp \
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
    widgets/DecompilerWidget.cpp \
    widgets/VisualNavbar.cpp \
    widgets/GraphView.cpp \
    dialogs/preferences/PreferencesDialog.cpp \
    dialogs/preferences/AppearanceOptionsWidget.cpp \
    dialogs/preferences/GraphOptionsWidget.cpp \
    dialogs/preferences/PreferenceCategory.cpp \
    dialogs/preferences/InitializationFileEditor.cpp \
    widgets/QuickFilterView.cpp \
    widgets/ClassesWidget.cpp \
    widgets/ResourcesWidget.cpp \
    widgets/VTablesWidget.cpp \
    widgets/TypesWidget.cpp \
    widgets/HeadersWidget.cpp \
    widgets/SearchWidget.cpp \
    CutterApplication.cpp \
    common/PythonAPI.cpp \
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
    widgets/ThreadsWidget.cpp \
    widgets/ProcessesWidget.cpp \
    widgets/BacktraceWidget.cpp \
    dialogs/MapFileDialog.cpp \
    common/CommandTask.cpp \
    common/ProgressIndicator.cpp \
    common/R2Task.cpp \
    dialogs/R2TaskDialog.cpp \
    widgets/DebugActions.cpp \
    widgets/MemoryMapWidget.cpp \
    dialogs/preferences/DebugOptionsWidget.cpp \
    dialogs/preferences/PluginsOptionsWidget.cpp \
    widgets/BreakpointWidget.cpp \
    dialogs/BreakpointsDialog.cpp \
    dialogs/AttachProcDialog.cpp \
    widgets/RegisterRefsWidget.cpp \
    dialogs/SetToDataDialog.cpp \
    dialogs/EditVariablesDialog.cpp \
    dialogs/EditFunctionDialog.cpp \
    widgets/CutterTreeView.cpp \
    widgets/ComboQuickFilterView.cpp \
    dialogs/HexdumpRangeDialog.cpp \
    common/QtResImporter.cpp \
    common/CutterSeekable.cpp \
    common/RefreshDeferrer.cpp \
    dialogs/WelcomeDialog.cpp \
    common/RunScriptTask.cpp \
    dialogs/EditMethodDialog.cpp \
    dialogs/TypesInteractionDialog.cpp \
    widgets/SdbWidget.cpp \
    common/PythonManager.cpp \
    plugins/PluginManager.cpp \
    common/BasicBlockHighlighter.cpp \
    common/BasicInstructionHighlighter.cpp \
    dialogs/LinkTypeDialog.cpp \
    widgets/ColorPicker.cpp \
    common/ColorThemeWorker.cpp \
    widgets/ColorThemeComboBox.cpp \
    widgets/ColorThemeListView.cpp \
    dialogs/preferences/ColorThemeEditDialog.cpp \
    common/UpdateWorker.cpp \
    widgets/MemoryDockWidget.cpp \
    common/CrashHandler.cpp \
    common/BugReporting.cpp \
    common/HighDpiPixmap.cpp \
    widgets/GraphGridLayout.cpp \
    widgets/HexWidget.cpp \
    common/SelectionHighlight.cpp \
    common/Decompiler.cpp \
    menus/AddressableItemContextMenu.cpp \
    common/AddressableItemModel.cpp \
    widgets/ListDockWidget.cpp \
    dialogs/MultitypeFileSaveDialog.cpp \
    widgets/BoolToggleDelegate.cpp \
    common/IOModesController.cpp \
    common/SettingsUpgrade.cpp \
    dialogs/LayoutManager.cpp \
    common/CutterLayout.cpp

GRAPHVIZ_SOURCES = \
    widgets/GraphvizLayout.cpp

HEADERS  += \
    core/Cutter.h \
    core/CutterCommon.h \
    core/CutterDescriptions.h \
    dialogs/EditStringDialog.h \
    dialogs/WriteCommandsDialogs.h \
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
    dialogs/RemoteDebugDialog.h \
    dialogs/NativeDebugDialog.h \
    dialogs/XrefsDialog.h \
    common/Helpers.h \
    common/HexAsciiHighlighter.h \
    common/HexHighlighter.h \
    core/MainWindow.h \
    common/Highlighter.h \
    common/MdHighlighter.h \
    common/DirectionalComboBox.h \
    dialogs/InitialOptionsDialog.h \
    dialogs/NewFileDialog.h \
    common/AnalTask.h \
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
    widgets/DecompilerWidget.h \
    widgets/VisualNavbar.h \
    widgets/GraphView.h \
    dialogs/preferences/PreferencesDialog.h \
    dialogs/preferences/AppearanceOptionsWidget.h \
    dialogs/preferences/PreferenceCategory.h \
    dialogs/preferences/GraphOptionsWidget.h \
    dialogs/preferences/InitializationFileEditor.h \
    widgets/QuickFilterView.h \
    widgets/ClassesWidget.h \
    widgets/ResourcesWidget.h \
    CutterApplication.h \
    widgets/VTablesWidget.h \
    widgets/TypesWidget.h \
    widgets/HeadersWidget.h \
    widgets/SearchWidget.h \
    common/PythonAPI.h \
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
    widgets/ThreadsWidget.h \
    widgets/ProcessesWidget.h \
    widgets/BacktraceWidget.h \
    dialogs/MapFileDialog.h \
    common/StringsTask.h \
    common/FunctionsTask.h \
    common/CommandTask.h \
    common/ProgressIndicator.h \
    plugins/CutterPlugin.h \
    common/R2Task.h \
    dialogs/R2TaskDialog.h \
    widgets/DebugActions.h \
    widgets/MemoryMapWidget.h \
    dialogs/preferences/DebugOptionsWidget.h \
    dialogs/preferences/PluginsOptionsWidget.h \
    widgets/BreakpointWidget.h \
    dialogs/BreakpointsDialog.h \
    dialogs/AttachProcDialog.h \
    widgets/RegisterRefsWidget.h \
    dialogs/SetToDataDialog.h \
    common/InitialOptions.h \
    dialogs/EditVariablesDialog.h \
    dialogs/EditFunctionDialog.h \
    widgets/CutterTreeView.h \
    widgets/ComboQuickFilterView.h \
    dialogs/HexdumpRangeDialog.h \
    common/QtResImporter.h \
    common/CutterSeekable.h \
    common/RefreshDeferrer.h \
    dialogs/WelcomeDialog.h \
    common/RunScriptTask.h \
    common/Json.h \
    dialogs/EditMethodDialog.h \
    common/CrashHandler.h \
    dialogs/TypesInteractionDialog.h \
    widgets/SdbWidget.h \
    common/PythonManager.h \
    plugins/PluginManager.h \
    common/BasicBlockHighlighter.h \
    common/BasicInstructionHighlighter.h \
    common/UpdateWorker.h \
    widgets/ColorPicker.h \
    common/ColorThemeWorker.h \
    widgets/ColorThemeComboBox.h \
    widgets/MemoryDockWidget.h \
    widgets/ColorThemeListView.h \
    dialogs/preferences/ColorThemeEditDialog.h \
    dialogs/LinkTypeDialog.h \
    common/BugReporting.h \
    common/HighDpiPixmap.h \
    widgets/GraphLayout.h \
    widgets/HexWidget.h \
    common/SelectionHighlight.h \
    common/Decompiler.h \
    menus/AddressableItemContextMenu.h \
    common/AddressableItemModel.h \
    widgets/ListDockWidget.h \
    widgets/AddressableItemList.h \
    dialogs/MultitypeFileSaveDialog.h \
    widgets/BoolToggleDelegate.h \
    common/IOModesController.h \
    common/SettingsUpgrade.h \
    dialogs/LayoutManager.h \
    common/CutterLayout.h

GRAPHVIZ_HEADERS = widgets/GraphGridLayout.h

FORMS    += \
    dialogs/AboutDialog.ui \
    dialogs/EditStringDialog.ui \
    dialogs/Base64EnDecodedWriteDialog.ui \
    dialogs/DuplicateFromOffsetDialog.ui \
    dialogs/IncrementDecrementDialog.ui \
    dialogs/preferences/AsmOptionsWidget.ui \
    dialogs/CommentsDialog.ui \
    dialogs/EditInstructionDialog.ui \
    dialogs/FlagDialog.ui \
    dialogs/RenameDialog.ui \
    dialogs/RemoteDebugDialog.ui \
    dialogs/NativeDebugDialog.ui \
    dialogs/XrefsDialog.ui \
    dialogs/NewfileDialog.ui \
    dialogs/InitialOptionsDialog.ui \
    dialogs/EditFunctionDialog.ui \
    core/MainWindow.ui \
    widgets/ConsoleWidget.ui \
    widgets/Dashboard.ui \
    widgets/EntrypointWidget.ui \
    widgets/FlagsWidget.ui \
    widgets/StringsWidget.ui \
    widgets/HexdumpWidget.ui \
    dialogs/SaveProjectDialog.ui \
    dialogs/preferences/PreferencesDialog.ui \
    dialogs/preferences/AppearanceOptionsWidget.ui \
    dialogs/preferences/GraphOptionsWidget.ui \
    dialogs/preferences/InitializationFileEditor.ui \
    widgets/QuickFilterView.ui \
    widgets/DecompilerWidget.ui \
    widgets/ClassesWidget.ui \
    widgets/VTablesWidget.ui \
    widgets/TypesWidget.ui \
    widgets/SearchWidget.ui \
    dialogs/R2PluginsDialog.ui \
    dialogs/VersionInfoDialog.ui \
    widgets/ZignaturesWidget.ui \
    dialogs/AsyncTaskDialog.ui \
    dialogs/R2TaskDialog.ui \
    widgets/StackWidget.ui \
    widgets/RegistersWidget.ui \
    widgets/ThreadsWidget.ui \
    widgets/ProcessesWidget.ui \
    widgets/BacktraceWidget.ui \
    dialogs/MapFileDialog.ui \
    dialogs/preferences/DebugOptionsWidget.ui \
    widgets/BreakpointWidget.ui \
    dialogs/BreakpointsDialog.ui \
    dialogs/AttachProcDialog.ui \
    widgets/RegisterRefsWidget.ui \
    dialogs/SetToDataDialog.ui \
    dialogs/EditVariablesDialog.ui \
    widgets/CutterTreeView.ui \
    widgets/ComboQuickFilterView.ui \
    dialogs/HexdumpRangeDialog.ui \
    dialogs/WelcomeDialog.ui \
    dialogs/EditMethodDialog.ui \
    dialogs/TypesInteractionDialog.ui \
    widgets/SdbWidget.ui \
    dialogs/LinkTypeDialog.ui \
    widgets/ColorPicker.ui \
    dialogs/preferences/ColorThemeEditDialog.ui \
    widgets/ListDockWidget.ui \
    dialogs/LayoutManager.ui

RESOURCES += \
    resources.qrc \
    themes/native/native.qrc \
    themes/qdarkstyle/dark.qrc \
    themes/midnight/midnight.qrc \
    themes/lightstyle/light.qrc


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

    INSTALLS += target share_appdata share_applications share_pixmaps

    # Triggered for example by 'qmake APPIMAGE=1'
    !isEmpty(APPIMAGE){
        appimage_root.path = /
        appimage_root.files = $$icon_file $$desktop_file

        INSTALLS += appimage_root
        DEFINES += APPIMAGE
    }
}
