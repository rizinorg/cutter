#ifndef CUTTERSAMPLEPLUGIN_H
#define CUTTERSAMPLEPLUGIN_H

#include <QObject>
#include <QtPlugin>
#include "CutterPlugin.h"

class CutterSamplePlugin : public QObject, CutterPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.radare.cutter.plugins.CutterPlugin")
    Q_INTERFACES(CutterPlugin)

public:
    void setupPlugin(CutterCore *core) override;
};


#endif // CUTTERSAMPLEPLUGIN_H
