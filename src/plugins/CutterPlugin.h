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
    virtual void setupPlugin() = 0;
    virtual void setupInterface(MainWindow *main) = 0;

    virtual QString getName() const = 0;
    virtual QString getAuthor() const = 0;
    virtual QString getDescription() const = 0;
    virtual QString getVersion() const = 0;
};

#define CutterPlugin_iid "org.radare.cutter.plugins.CutterPlugin"

Q_DECLARE_INTERFACE(CutterPlugin, CutterPlugin_iid)

#endif // CUTTERPLUGIN_H
