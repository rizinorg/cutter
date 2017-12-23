TEMPLATE = app

TARGET = cutter

# The application version
VERSION = 1.0

ICON = img/cutter.icns

QT += core gui widgets svg
QT_CONFIG -= no-pkg-config
CONFIG += c++11

# Define the preprocessor macro to get the application version in our application.
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

INCLUDEPATH *= .

win32 {
    # Generate debug symbols in release mode
    QMAKE_CXXFLAGS_RELEASE += -Zi   # Compiler
    QMAKE_LFLAGS_RELEASE += /DEBUG  # Linker
}

macx {
    QMAKE_CXXFLAGS = -mmacosx-version-min=10.7 -std=gnu0x -stdlib=libc++
}


unix:exists(/usr/local/include/libr) {
    INCLUDEPATH += /usr/local/include/libr
}

SOURCES += \
    main.cpp \
    cutter.cpp \
    widgets/DisassemblerGraphView.cpp \
    utils/RichTextPainter.cpp \
    dialogs/OptionsDialog.cpp \
    dialogs/AboutDialog.cpp \
    dialogs/CommentsDialog.cpp \
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
    AnalThread.cpp \
    widgets/CommentsWidget.cpp \
    widgets/ConsoleWidget.cpp \
    widgets/Dashboard.cpp \
    widgets/EntrypointWidget.cpp \
    widgets/ExportsWidget.cpp \
    widgets/FlagsWidget.cpp \
    widgets/FunctionsWidget.cpp \
    widgets/ImportsWidget.cpp \
    widgets/Notepad.cpp \
    widgets/Omnibar.cpp \
    widgets/PieView.cpp \
    widgets/RelocsWidget.cpp \
    widgets/SdbDock.cpp \
    widgets/SectionsDock.cpp \
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
    widgets/QuickFilterView.cpp \
    widgets/ClassesWidget.cpp

HEADERS  += \
    cutter.h \
    widgets/DisassemblerGraphView.h \
    utils/RichTextPainter.h \
    utils/CachedFontMetrics.h \
    dialogs/AboutDialog.h \
    dialogs/preferences/AsmOptionsWidget.h \
    dialogs/CommentsDialog.h \
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
    AnalThread.h \
    widgets/CommentsWidget.h \
    widgets/ConsoleWidget.h \
    widgets/Dashboard.h \
    widgets/EntrypointWidget.h \
    widgets/ExportsWidget.h \
    widgets/FlagsWidget.h \
    widgets/FunctionsWidget.h \
    widgets/ImportsWidget.h \
    widgets/Notepad.h \
    widgets/Omnibar.h \
    widgets/PieView.h \
    widgets/RelocsWidget.h \
    widgets/SdbDock.h \
    widgets/SectionsDock.h \
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
    dialogs/preferences/GraphOptionsWidget.h \
    widgets/QuickFilterView.h \
    widgets/ClassesWidget.h

FORMS    += \
    dialogs/AboutDialog.ui \
    dialogs/preferences/AsmOptionsWidget.ui \
    dialogs/CommentsDialog.ui \
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
    widgets/Notepad.ui \
    widgets/SdbDock.ui \
    widgets/RelocsWidget.ui \
    widgets/SectionsDock.ui \
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
    widgets/ClassesWidget.ui

RESOURCES += \
    resources.qrc

DISTFILES += cutter.astylerc


include(lib_radare2.pri)


# 'make install' for AppImage
unix {
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }

    icon_file = img/cutter.svg

    share_pixmaps.path = $$PREFIX/share/pixmaps
    share_pixmaps.files = $$icon_file


    desktop_file = cutter.desktop

    # built-in no need for files atm
    target.path = $$PREFIX/bin

    share_applications.path = $$PREFIX/share/applications
    share_applications.files = $$desktop_file

    INSTALLS += target share_applications share_pixmaps

    # Triggered for example by 'qmake APPIMAGE=1'
    !isEmpty(APPIMAGE){
        appimage_root.path = /
        appimage_root.files = $$icon_file $$desktop_file

        INSTALLS += appimage_root
        DEFINES += APPIMAGE
    }
}
