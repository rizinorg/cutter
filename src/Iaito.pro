#-------------------------------------------------
#
# Project created by QtCreator 2014-02-27T11:31:27
#
#-------------------------------------------------

ICON = img/Enso.icns

# No idea what this does exactly
TEMPLATE = app

# The application version
VERSION = 1.0-dev

# Define the preprocessor macro to get the application version in our application.
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

QT       += core gui webkit webkitwidgets
QT_CONFIG -= no-pkg-config

macx {
	QMAKE_CXXFLAGS = -mmacosx-version-min=10.7 -std=gnu0x -stdlib=libc+
		EXTSO=dylib
} else {
	win32 {
		EXTSO=dll
	} else {
		EXTSO=so
	}
}
CONFIG += c++11


greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Iaito
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    newfiledialog.cpp \
    optionsdialog.cpp \
    highlighter.cpp \
    qrcore.cpp \
    createnewdialog.cpp \
    hexascii_highlighter.cpp \
    webserverthread.cpp \
    widgets/pieview.cpp \
    widgets/sectionswidget.cpp \
    widgets/codegraphic.cpp \
    widgets/notepad.cpp \
    mdhighlighter.cpp \
    widgets/functionswidget.cpp \
    dialogs/renamedialog.cpp \
    dialogs/aboutdialog.cpp \
    widgets/importswidget.cpp \
    widgets/symbolswidget.cpp \
    widgets/relocswidget.cpp \
    widgets/commentswidget.cpp \
    widgets/stringswidget.cpp \
    widgets/flagswidget.cpp \
    widgets/memwidget/memorywidget.cpp \
    qrdisasm.cpp \
    widgets/sdbdock.cpp \
    analthread.cpp \
    dialogs/commentsdialog.cpp \
    widgets/sidebar.cpp \
    helpers.cpp \
    widgets/omnibar.cpp \
    widgets/dashboard.cpp \
    dialogs/xrefsdialog.cpp \
    hexhighlighter.cpp

HEADERS  += mainwindow.h \
    newfiledialog.h \
    optionsdialog.h \
    highlighter.h \
    qrcore.h \
    createnewdialog.h \
    hexascii_highlighter.h \
    webserverthread.h \
    widgets/pieview.h \
    widgets/sectionswidget.h \
    widgets/codegraphic.h \
    widgets/notepad.h \
    mdhighlighter.h \
    widgets/functionswidget.h \
    dialogs/renamedialog.h \
    dialogs/aboutdialog.h \
    widgets/importswidget.h \
    widgets/symbolswidget.h \
    widgets/relocswidget.h \
    widgets/commentswidget.h \
    widgets/stringswidget.h \
    widgets/flagswidget.h \
    widgets/memwidget/memorywidget.h \
    qrdisasm.h \
    widgets/sdbdock.h \
    analthread.h \
    dialogs/commentsdialog.h \
    widgets/sidebar.h \
    helpers.h \
    widgets/omnibar.h \
    widgets/dashboard.h \
    dialogs/xrefsdialog.h \
    widgets/banned.h \
    hexhighlighter.h

FORMS    += mainwindow.ui \
    newfiledialog.ui \
    optionsdialog.ui \
    createnewdialog.ui \
    widgets/notepad.ui \
    widgets/functionswidget.ui \
    dialogs/aboutdialog.ui \
    dialogs/renamedialog.ui \
    widgets/importswidget.ui \
    widgets/symbolswidget.ui \
    widgets/relocswidget.ui \
    widgets/commentswidget.ui \
    widgets/stringswidget.ui \
    widgets/flagswidget.ui \
    widgets/memwidget/memorywidget.ui \
    widgets/sdbdock.ui \
    dialogs/commentsdialog.ui \
    widgets/sidebar.ui \
    widgets/dashboard.ui \
    dialogs/xrefsdialog.ui

RESOURCES += \
    resources.qrc

#INCLUDEPATH += /usr/local/radare2/osx/include/libr
INCLUDEPATH += /usr/local/include/libr
INCLUDEPATH += /usr/include/libr
#LIBS += -L/usr/local/radare2/osx/lib -lr_core -lr_config -lr_cons -lr_io -lr_util -lr_flag -lr_asm -lr_debug -lr_hash -lr_bin -lr_lang -lr_io -lr_anal -lr_parse -lr_bp -lr_egg -lr_reg -lr_search -lr_syscall -lr_socket -lr_fs -lr_magic -lr_crypto
LIBS += -L/usr/local/lib -lr_core -lr_config -lr_cons -lr_io -lr_util -lr_flag -lr_asm -lr_debug -lr_hash -lr_bin -lr_lang -lr_io -lr_anal -lr_parse -lr_bp -lr_egg -lr_reg -lr_search -lr_syscall -lr_socket -lr_fs -lr_magic -lr_crypto
