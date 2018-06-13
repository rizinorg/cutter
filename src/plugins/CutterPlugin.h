#ifndef CUTTERPLUGIN_H
#define CUTTERPLUGIN_H

#include "Cutter.h"

class CutterPlugin 
{
public:
    virtual ~CutterPlugin() {}
    virtual void setupPlugin(CutterCore *core) = 0;
};

#define CutterPlugin_iid "org.radare.cutter.plugins.CutterPlugin"

Q_DECLARE_INTERFACE(CutterPlugin, CutterPlugin_iid)

#endif // CUTTERPLUGIN_H
