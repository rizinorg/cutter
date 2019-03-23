TEMPLATE = app

TARGET = Cutter

CUTTER_VERSION_MAJOR = 1
CUTTER_VERSION_MINOR = 8
CUTTER_VERSION_PATCH = 0

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

!defined(CUTTER_ENABLE_PYTHON, var)             CUTTER_ENABLE_PYTHON=false
equals(CUTTER_ENABLE_PYTHON, true)              CONFIG += CUTTER_ENABLE_PYTHON

!defined(CUTTER_ENABLE_PYTHON_BINDINGS, var)    CUTTER_ENABLE_PYTHON_BINDINGS=false
equals(CUTTER_ENABLE_PYTHON, true) {
    equals(CUTTER_ENABLE_PYTHON_BINDINGS, true) {
        CONFIG += CUTTER_ENABLE_PYTHON_BINDINGS
        !defined(SHIBOKEN_EXECUTABLE, var) SHIBOKEN_EXECUTABLE=shiboken2
    }
}

!defined(CUTTER_ENABLE_JUPYTER, var)            CUTTER_ENABLE_JUPYTER=false
equals(CUTTER_ENABLE_PYTHON, true) {
    equals(CUTTER_ENABLE_JUPYTER, true)         CONFIG += CUTTER_ENABLE_JUPYTER
}

!defined(CUTTER_ENABLE_QTWEBENGINE, var)        CUTTER_ENABLE_QTWEBENGINE=false
equals(CUTTER_ENABLE_JUPYTER, true) {
    equals(CUTTER_ENABLE_QTWEBENGINE, true)     CONFIG += CUTTER_ENABLE_QTWEBENGINE
}

!defined(CUTTER_BUNDLE_R2_APPBUNDLE, var)       CUTTER_BUNDLE_R2_APPBUNDLE=false
equals(CUTTER_BUNDLE_R2_APPBUNDLE, true)        CONFIG += CUTTER_BUNDLE_R2_APPBUNDLE

!defined(CUTTER_APPVEYOR_R2DEC, var)            CUTTER_APPVEYOR_R2DEC=false
equals(CUTTER_APPVEYOR_R2DEC, true)             CONFIG += CUTTER_APPVEYOR_R2DEC

!defined(CUTTER_APPVEYOR_R2DEC, var)            CUTTER_APPVEYOR_R2DEC=false

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

CUTTER_ENABLE_JUPYTER {
    message("Jupyter support enabled.")
    DEFINES += CUTTER_ENABLE_JUPYTER
} else {
    message("Jupyter support disabled. (requires CUTTER_ENABLE_PYTHON=true)")
}

CUTTER_ENABLE_QTWEBENGINE {
    message("QtWebEngine support enabled.")
    DEFINES += CUTTER_ENABLE_QTWEBENGINE
    QT += webenginewidgets
} else {
    message("QtWebEngine support disabled. (requires CUTTER_ENABLE_JUPYTER=true)")
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
        pythonpath = $$replace(PYTHON_EXECUTABLE, ".exe ", ".exe;")
        pythonpath = $$section(pythonpath, ";", 0, 0)
        pythonpath = $$clean_path($$dirname(pythonpath))
        LIBS += -L$${pythonpath} -L$${pythonpath}/libs -lpython3
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
            BINDINGS_SRC_LIST_CMD = "$${PYTHON_EXECUTABLE} bindings/src_list.py"
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


macx:CUTTER_BUNDLE_R2_APPBUNDLE {
    message("Using r2 rom AppBundle")
    DEFINES += MACOS_R2_BUNDLED
}

CUTTER_APPVEYOR_R2DEC {
    message("Appveyor r2dec")
    DEFINES += CUTTER_APPVEYOR_R2DEC
}

QMAKE_SUBSTITUTES += CutterConfig.h.in

include(common/Common.pri)
include(core/Core.pri)
include(dialogs/Dialogs.pri)
include(menus/Menus.pri)
include(plugins/Plugins.pri)
include(widgets/Widgets.pri)

SOURCES += \
    Main.cpp \
    CutterApplication.cpp \

HEADERS  += \
    CutterApplication.h \

RESOURCES += \
    resources.qrc \
    themes/qdarkstyle/style.qrc


DISTFILES += Cutter.astylerc \
    common/Common.pri \
    core/Core.pri \
    dialogs/Dialogs.pri \
    menus/Menus.pri \
    widgets/Widgets.pri

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
