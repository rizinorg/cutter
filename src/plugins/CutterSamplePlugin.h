#ifndef CUTTERSAMPLEPLUGIN_H
#define CUTTERSAMPLEPLUGIN_H

#include <QObject>
#include <QtPlugin>
#include "CutterPlugin.h"

class CutterSamplePlugin : public QWidget, CutterPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.radare.cutter.plugins.CutterPlugin")
    Q_INTERFACES(CutterPlugin)

public:
    void setupPlugin(CutterCore *core) override;
    void setupInterface(QWidget *parent) override;
};


#endif // CUTTERSAMPLEPLUGIN_H
