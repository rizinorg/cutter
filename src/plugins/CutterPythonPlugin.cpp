
#include <Python.h>

#include "CutterPythonPlugin.h"

CutterPythonPlugin::CutterPythonPlugin(PyObject* pluginModule)
{
    this->pluginModule = pluginModule;
}

void CutterPythonPlugin::setupPlugin(CutterCore *core)
{
    Q_UNUSED(core)

    //PyObject *pInstance = PyObject_GetAttrString(pluginModule, "plugin");
    PyObject *pInstance = Python()->getAttrStringSafe(pluginModule, "plugin");
    if (!pInstance) {
        qWarning() << "Cannot find plugin instance.";
        return;
    }

    PyObject *pName = PyObject_GetAttrString(pInstance, "name");
    qDebug() << "pname" << pName;
    if (pName) {
        this->name = QString(PyUnicode_AS_DATA(pName));
        qDebug() << "OK COOL" << this->name;
    }
}

CutterDockWidget* CutterPythonPlugin::setupInterface(MainWindow *main, QAction *action)
{
    Q_UNUSED(main)
    Q_UNUSED(action)

    return nullptr;
}
