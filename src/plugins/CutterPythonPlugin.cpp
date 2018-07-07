
#include <Python.h>

#include "CutterPythonPlugin.h"

CutterPythonPlugin::CutterPythonPlugin(PyObject *pluginModule)
{
    this->pluginModule = pluginModule;

    if (!pluginModule) {
        qWarning() << "Could not find plugin module.";
        return;
    }
    pInstance = PyObject_GetAttrString(pluginModule, "plugin");
    if (!pInstance) {
        qWarning() << "Cannot find plugin instance.";
        return;
    }

    auto getStringAttr = [this](const char *attrName) -> QString {
        if (!PyObject_HasAttrString(pInstance, attrName)) {
            return QString();
        }
        PyObject *attr = PyObject_GetAttrString(pInstance, attrName);
        if (!attr) {
            PyErr_Print();
            return QString();
        }
        if (!PyUnicode_Check(attr)) {
            return QString();
        }
        return QString::fromUtf8(PyUnicode_AsUTF8(attr));
    };

    name = getStringAttr("name");
    description = getStringAttr("description");
    version = getStringAttr("version");
    author = getStringAttr("author");
}

CutterPythonPlugin::~CutterPythonPlugin()
{
    Python()->restoreThread();
    if (pInstance) {
        Py_DECREF(pInstance);
    }
    if (pluginModule) {
        Py_DECREF(pluginModule);
    }
    Python()->saveThread();
}

void CutterPythonPlugin::setupPlugin(CutterCore *core)
{
    Q_UNUSED(core)

    Python()->restoreThread();
    // Check setupPlugin method exists
    PyObject *setupPlugin = PyObject_GetAttrString(pInstance, "setupPlugin");
    if (!setupPlugin) {
        qWarning() << "Cannot find setupPlugin method.";
        Python()->saveThread();
        return;
    }
    Py_DECREF(setupPlugin);

    // Call that method
    PyObject *result = PyObject_CallMethod(pInstance, "setupPlugin", nullptr);
    if (!result) {
        qWarning() << "Error in setupPlugin().";
        PyErr_Print();
        Python()->saveThread();
    }
    Py_DECREF(result);

    this->name = getAttributeFromPython("name");
    this->description = getAttributeFromPython("description");
    this->version = getAttributeFromPython("version");
    this->author = getAttributeFromPython("author");

    Python()->saveThread();
}

QString CutterPythonPlugin::getAttributeFromPython(const char *attribute)
{
    QString result;
    PyObject *pName = PyObject_GetAttrString(pInstance, attribute);
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

    PyObject *pWidget = nullptr;
    Python()->restoreThread();
    pWidget = PyObject_CallMethod(pInstance, "setupInterface", nullptr);

    if (!pWidget) {
        qWarning() << "Error in setupInterface().";
        PyErr_Print();
        Python()->saveThread();
        return nullptr;
    }

    if (!PyLong_Check(pWidget)) {
        qWarning() << "Value returned by setupInterface() is not PyLong.";
        Python()->saveThread();
        return nullptr;
    }

    auto dockWidget = reinterpret_cast<QDockWidget *>(PyLong_AsLong(pWidget));
    printf("plugin gave me this: %s\n", dockWidget->objectName().toLocal8Bit().constData());

    Python()->saveThread();

    return nullptr;
}
