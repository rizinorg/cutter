TEMPLATE = app

TARGET = Cutter

CUTTER_VERSION_MAJOR = 1
CUTTER_VERSION_MINOR = 8
CUTTER_VERSION_PATCH = 1

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

!defined(CUTTER_ENABLE_CRASH_REPORTS, var)        CUTTER_ENABLE_CRASH_REPORTS=false
equals(CUTTER_ENABLE_CRASH_REPORTS, true)         CONFIG += CUTTER_ENABLE_CRASH_REPORTS
!defined(CUTTER_ENABLE_PYTHON, var)             CUTTER_ENABLE_PYTHON=false
equals(CUTTER_ENABLE_PYTHON, true)              CONFIG += CUTTER_ENABLE_PYTHON

!defined(CUTTER_ENABLE_PYTHON_BINDINGS, var)    CUTTER_ENABLE_PYTHON_BINDINGS=false
equals(CUTTER_ENABLE_PYTHON, true) {
    equals(CUTTER_ENABLE_PYTHON_BINDINGS, true) {
        CONFIG += CUTTER_ENABLE_PYTHON_BINDINGS
        !defined(SHIBOKEN_EXECUTABLE, var) SHIBOKEN_EXECUTABLE=shiboken2
    }
}

!defined(CUTTER_BUNDLE_R2_APPBUNDLE, var)       CUTTER_BUNDLE_R2_APPBUNDLE=false
equals(CUTTER_BUNDLE_R2_APPBUNDLE, true)        CONFIG += CUTTER_BUNDLE_R2_APPBUNDLE

!defined(CUTTER_APPVEYOR_R2DEC, var)            CUTTER_APPVEYOR_R2DEC=false
equals(CUTTER_APPVEYOR_R2DEC, true)             CONFIG += CUTTER_APPVEYOR_R2DEC

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

INCLUDEPATH *= . core widgets dialogs common plugins

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
        win32:SHIBOKEN_OPTIONS += --avoid-protected-hack
        bindings.target = bindings_target
        bindings.commands = "$${SHIBOKEN_EXECUTABLE}" $${SHIBOKEN_OPTIONS}
        QMAKE_EXTRA_TARGETS += bindings
        PRE_TARGETDEPS += bindings_target
        GENERATED_SOURCES += $${BINDINGS_SOURCE}

        INCLUDEPATH += "$${BINDINGS_BUILD_DIR}/CutterBindings"

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

QMAKE_SUBSTITUTES += CutterConfig.h.in

SOURCES += \
    Main.cpp \
    core/Cutter.cpp \
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
    core/MainWindow.cpp \
    common/Helpers.cpp \
    common/HexAsciiHighlighter.cpp \
    common/HexHighlighter.cpp \
    common/Highlighter.cpp \
    common/MdHighlighter.cpp \
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
    widgets/BacktraceWidget.cpp \
    dialogs/OpenFileDialog.cpp \
    common/CommandTask.cpp \
    common/ProgressIndicator.cpp \
    common/R2Task.cpp \
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
    dialogs/LoadNewTypesDialog.cpp \
    widgets/SdbWidget.cpp \
    common/PythonManager.cpp \
    plugins/PluginManager.cpp \
    common/BasicBlockHighlighter.cpp \
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
    widgets/HexWidget.cpp

HEADERS  += \
    core/Cutter.h \
    core/CutterCommon.h \
    core/CutterDescriptions.h \
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
    core/MainWindow.h \
    common/Highlighter.h \
    common/MdHighlighter.h \
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
    dialogs/LoadNewTypesDialog.h \
    widgets/SdbWidget.h \
    common/PythonManager.h \
    plugins/PluginManager.h \
    common/BasicBlockHighlighter.h \
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
    widgets/GraphGridLayout.h \
    widgets/HexWidget.h

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
    core/MainWindow.ui \
    widgets/CommentsWidget.ui \
    widgets/ConsoleWidget.ui \
    widgets/Dashboard.ui \
    widgets/EntrypointWidget.ui \
    widgets/FlagsWidget.ui \
    widgets/ExportsWidget.ui \
    widgets/FunctionsWidget.ui \
    widgets/ImportsWidget.ui \
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
    widgets/CutterTreeView.ui \
    widgets/ComboQuickFilterView.ui \
    dialogs/HexdumpRangeDialog.ui \
    dialogs/WelcomeDialog.ui \
    dialogs/EditMethodDialog.ui \
    dialogs/LoadNewTypesDialog.ui \
    widgets/SdbWidget.ui \
    dialogs/LinkTypeDialog.ui \
    widgets/ColorPicker.ui \
    dialogs/preferences/ColorThemeEditDialog.ui

RESOURCES += \
    resources.qrc \
    themes/native/native.qrc \
    themes/qdarkstyle/style.qrc \
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
