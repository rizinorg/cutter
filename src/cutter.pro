TEMPLATE = app

TARGET = cutter

# The application version
win32 {
    VERSION = 1.0
    # Generate debug symbols in release mode
    QMAKE_CXXFLAGS_RELEASE += -Zi   # Compiler
    QMAKE_LFLAGS_RELEASE += /DEBUG  # Linker
} else {
    VERSION = 1.0-dev
}

ICON = img/Enso.icns

QT += core gui widgets webengine webenginewidgets

QT_CONFIG -= no-pkg-config

CONFIG += debug c++11

# Define the preprocessor macro to get the application version in our application.
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

macx {
    QMAKE_CXXFLAGS = -mmacosx-version-min=10.7 -std=gnu0x -stdlib=libc++
}

INCLUDEPATH *= .

unix:exists(/usr/local/include/libr) {
    INCLUDEPATH += /usr/local/include/libr
}

SOURCES += \
    main.cpp \
    cutter.cpp \
    widgets/pieview.cpp \
    widgets/sectionswidget.cpp \
    widgets/codegraphic.cpp \
    widgets/notepad.cpp \
    widgets/functionswidget.cpp \
    widgets/importswidget.cpp \
    widgets/symbolswidget.cpp \
    widgets/relocswidget.cpp \
    widgets/commentswidget.cpp \
    widgets/stringswidget.cpp \
    widgets/flagswidget.cpp \
    widgets/exportswidget.cpp \
    widgets/sdbdock.cpp \
    analthread.cpp \
    widgets/sidebar.cpp \
    widgets/omnibar.cpp \
    widgets/dashboard.cpp \
    widgets/sectionsdock.cpp \
    widgets/consolewidget.cpp \
    radarewebserver.cpp \
    widgets/entrypointwidget.cpp \
    widgets/DisassemblerGraphView.cpp \
    widgets/MemoryWidget.cpp \
    utils/RichTextPainter.cpp \
    dialogs/OptionsDialog.cpp \
    dialogs/AboutDialog.cpp \
    dialogs/AsmoptionsDialog.cpp \
    dialogs/CommentsDialog.cpp \
    dialogs/FlagDialog.cpp \
    dialogs/NewfileDialog.cpp \
    dialogs/RenameDialog.cpp \
    dialogs/XrefsDialog.cpp \
    MainWindow.cpp \
    utils/Helpers.cpp \
    utils/HexAsciiHighlighter.cpp \
    utils/HexHighlighter.cpp \
    utils/Highlighter.cpp \
    dialogs/CreatenewDialog.cpp \
    utils/MdHighlighter.cpp

HEADERS  += \
    widgets/pieview.h \
    widgets/sectionswidget.h \
    widgets/codegraphic.h \
    widgets/notepad.h \
    widgets/functionswidget.h \
    widgets/importswidget.h \
    widgets/symbolswidget.h \
    widgets/relocswidget.h \
    widgets/commentswidget.h \
    widgets/stringswidget.h \
    widgets/flagswidget.h \
    widgets/exportswidget.h \
    widgets/sdbdock.h \
    analthread.h \
    widgets/sidebar.h \
    widgets/omnibar.h \
    widgets/dashboard.h \
    widgets/sectionsdock.h \
    widgets/dockwidget.h \
    widgets/consolewidget.h \
    radarewebserver.h \
    widgets/entrypointwidget.h \
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
    dialogs/CreatenewDialog.h \
    utils/Helpers.h \
    utils/HexAsciiHighlighter.h \
    utils/HexHighlighter.h \
    MainWindow.h \
    utils/Highlighter.h \
    utils/MdHighlighter.h \
    dialogs/NewfileDialog.h \
    Settings.h \
    dialogs/OptionsDialog.h

FORMS    += \
    widgets/notepad.ui \
    widgets/functionswidget.ui \
    widgets/importswidget.ui \
    widgets/symbolswidget.ui \
    widgets/relocswidget.ui \
    widgets/commentswidget.ui \
    widgets/stringswidget.ui \
    widgets/flagswidget.ui \
    widgets/exportswidget.ui \
    widgets/sdbdock.ui \
    widgets/sidebar.ui \
    widgets/dashboard.ui \
    widgets/sectionsdock.ui \
    widgets/consolewidget.ui \
    widgets/entrypointwidget.ui \
    widgets/MemoryWidget.ui \
    dialogs/AboutDialog.ui \
    dialogs/AsmOptionsDialog.ui \
    dialogs/CommentsDialog.ui \
    dialogs/FlagDialog.ui \
    dialogs/RenameDialog.ui \
    dialogs/XrefsDialog.ui \
    dialogs/CreatenewDialog.ui \
    dialogs/NewfileDialog.ui \
    dialogs/OptionsDialog.ui \
    MainWindow.ui

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
