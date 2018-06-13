#ifndef CUTTERPLUGIN_H
#define CUTTERPLUGIN_H

class CutterPlugin;

#include "Cutter.h"

class CutterPlugin 
{
public:
    virtual ~CutterPlugin() {}
    virtual void setupPlugin(CutterCore *core) = 0;
    virtual void setupInterface(QWidget *parent) = 0;

    QString name;
    QString description;
    QString version;
    QString author;

protected:
    CutterCore *core;
};

#define CutterPlugin_iid "org.radare.cutter.plugins.CutterPlugin"

Q_DECLARE_INTERFACE(CutterPlugin, CutterPlugin_iid)

#endif // CUTTERPLUGIN_H
