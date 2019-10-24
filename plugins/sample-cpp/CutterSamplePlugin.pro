HEADERS        += ../CutterSamplePlugin.h ../CutterPlugin.h
INCLUDEPATH    += ../ ../../
SOURCES        += CutterSamplePlugin.cpp

QMAKE_CXXFLAGS += $$system("pkg-config --cflags r_core")

TEMPLATE        = lib
CONFIG         += plugin
QT             += widgets
TARGET          = PluginSample
