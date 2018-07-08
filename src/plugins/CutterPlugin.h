#ifndef CUTTERPLUGIN_H
#define CUTTERPLUGIN_H

class CutterPlugin;
class MainWindow;

#include "Cutter.h"
#include "widgets/CutterDockWidget.h"

class CutterPlugin 
{
public:
    virtual ~CutterPlugin() {}
    virtual void setupPlugin(CutterCore *core) = 0;
    virtual QDockWidget *setupInterface(MainWindow *main, QAction *action = nullptr) = 0;

    QString name;
    QString description;
    QString version;
    QString author;

protected:
    CutterCore *core;
    CutterDockWidget *dockable;
};

#define CutterPlugin_iid "org.radare.cutter.plugins.CutterPlugin"

Q_DECLARE_INTERFACE(CutterPlugin, CutterPlugin_iid)

#endif // CUTTERPLUGIN_H
