HEADERS        += CutterSamplePlugin.h CutterPlugin.h
INCLUDEPATH    += ../
SOURCES        += CutterSamplePlugin.cpp

# Needed for r_core include TODO cross platform
unix:exists(/usr/include/libr) {
    INCLUDEPATH += /usr/include/libr
}

TEMPLATE        = lib
CONFIG         += plugin
QT             += widgets
TARGET          = PluginSample
