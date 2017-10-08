TEMPLATE = app

TARGET = cutter

# The application version
VERSION = 1.0

ICON = img/Enso.icns

QT += core gui widgets webengine webenginewidgets
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
    widgets/MemoryWidget.cpp \
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
    dialogs/AsmOptionsDialog.cpp \
    dialogs/CreateNewDialog.cpp \
    dialogs/NewFileDialog.cpp \
    RadareWebServer.cpp \
    AnalThread.cpp \
    widgets/CodeGraphic.cpp \
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
    widgets/SymbolsWidget.cpp

HEADERS  += \
    cutter.h \
    widgets/DisassemblerGraphView.h \
    widgets/MemoryWidget.h \
    utils/RichTextPainter.h \
    utils/CachedFontMetrics.h \
    dialogs/AboutDialog.h \
    dialogs/AsmOptionsDialog.h \
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
    Settings.h \
    dialogs/OptionsDialog.h \
    dialogs/CreateNewDialog.h \
    dialogs/NewFileDialog.h \
    RadareWebServer.h \
    AnalThread.h \
    widgets/CodeGraphic.h \
    widgets/CommentsWidget.h \
    widgets/ConsoleWidget.h \
    widgets/Dashboard.h \
    widgets/DockWidget.h \
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
    widgets/SymbolsWidget.h

FORMS    += \
    widgets/MemoryWidget.ui \
    dialogs/AboutDialog.ui \
    dialogs/AsmOptionsDialog.ui \
    dialogs/CommentsDialog.ui \
    dialogs/FlagDialog.ui \
    dialogs/RenameDialog.ui \
    dialogs/XrefsDialog.ui \
    dialogs/NewfileDialog.ui \
    dialogs/OptionsDialog.ui \
    MainWindow.ui \
    dialogs/CreateNewDialog.ui \
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
    widgets/SymbolsWidget.ui

RESOURCES += \
    resources.qrc

DISTFILES += cutter.astylerc


include(lib_radare2.pri)


# 'make install' for AppImage
unix {
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }

    icon_file = img/cutter-small.png

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
        # UGLY work around for the logo name in cutter.desktop
        # Would be better to have a file called cutter.png in the first place
        system(cp img/cutter-small.png $$OUT_PWD/cutter-small.png)

        appimage_root.path = /
        appimage_root.files = $$OUT_PWD/cutter.png $$desktop_file

        INSTALLS += appimage_root
    }
}
