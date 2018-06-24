#ifndef CUTTERPYTHONPLUGIN_H
#define CUTTERPYTHONPLUGIN_H

#include "common/PythonManager.h"
#include "CutterPlugin.h"

class CutterPythonPlugin : public CutterPlugin
{
public:
    CutterPythonPlugin(PyObject* pluginModule);
    void setupPlugin(CutterCore *core);
    CutterDockWidget* setupInterface(MainWindow *main, QAction *action);

private:
    PyObject *pluginModule;
    QString getAttributeFromPython(PyObject *object, const char *attribute);
};

#endif // CUTTERPYTHONPLUGIN_H
