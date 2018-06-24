
#include <Python.h>

#include "CutterPythonPlugin.h"

CutterPythonPlugin::CutterPythonPlugin(PyObject* pluginModule)
{
    this->pluginModule = pluginModule;
}

void CutterPythonPlugin::setupPlugin(CutterCore *core)
{
    Q_UNUSED(core)

    if (!pluginModule) {
        qWarning() << "Could not find plugin module.";
        return;
    }

    Python()->restoreThread();
    PyObject *pInstance = PyObject_GetAttrString(pluginModule, "plugin");
    if (!pInstance) {
        qWarning() << "Cannot find plugin instance.";
        Python()->saveThread();
        return;
    }

    // Check setupPlugin method exists
    PyObject *setupPlugin = PyObject_GetAttrString(pInstance, "setupPlugin");
    if (!setupPlugin) {
        qWarning() << "Cannot find setupPlugin method.";
        Py_DECREF(pInstance);
        Python()->saveThread();
        return;
    }
    Py_DECREF(setupPlugin);

    // Call that method
    PyObject *result = PyObject_CallMethod(pInstance, "setupPlugin", nullptr);
    if (!result) {
        qWarning() << "Error in setupPlugin().";
        Py_DECREF(pInstance);
        Python()->saveThread();
    }
    Py_DECREF(result);

    this->name = getAttributeFromPython(pInstance, "name");
    this->description = getAttributeFromPython(pInstance, "description");
    this->version = getAttributeFromPython(pInstance, "version");
    this->author = getAttributeFromPython(pInstance, "author");

    Py_DECREF(pInstance);
    Python()->saveThread();
}

QString CutterPythonPlugin::getAttributeFromPython(PyObject *object, const char *attribute)
{
    QString result;
    PyObject *pName = PyObject_GetAttrString(object, attribute);
    if (pName) {
        result = QString(PyUnicode_AsUTF8(pName));
    }
    Py_DECREF(pName);

    return result;
}

CutterDockWidget* CutterPythonPlugin::setupInterface(MainWindow *main, QAction *action)
{
    Q_UNUSED(main)
    Q_UNUSED(action)

    return nullptr;
}
