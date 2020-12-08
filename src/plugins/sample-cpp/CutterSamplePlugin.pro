HEADERS        += ../CutterSamplePlugin.h ../CutterPlugin.h
INCLUDEPATH    += ../ ../../ ../../core ../../widgets
SOURCES        += CutterSamplePlugin.cpp

QMAKE_CXXFLAGS += $$system("pkg-config --cflags rz_core")

TEMPLATE        = lib
CONFIG         += plugin
QT             += widgets
TARGET          = PluginSample
