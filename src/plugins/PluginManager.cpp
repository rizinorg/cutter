
#include <cassert>

#ifdef CUTTER_ENABLE_PYTHON_BINDINGS
#include <Python.h>
#include <cutterbindings_python.h>
#include "PythonManager.h"
#endif

#include "PluginManager.h"
#include "CutterPlugin.h"

#include <QDir>
#include <QCoreApplication>
#include <QPluginLoader>
#include <QStandardPaths>

Q_GLOBAL_STATIC(PluginManager, uniqueInstance)

PluginManager *PluginManager::getInstance()
{
    return uniqueInstance;
}

PluginManager::PluginManager()
{
}

PluginManager::~PluginManager()
{
}

void PluginManager::loadPlugins()
{
    assert(plugins.isEmpty());

    QStringList locations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    if (locations.isEmpty()) {
        qCritical() << "Failed to get a standard path to load plugins from.";
        return;
    }
    QDir pluginsDir(locations.first());
    pluginsDir.mkpath(".");

    pluginsDir.mkdir("plugins");
    if (!pluginsDir.cd("plugins")) {
        return;
    }

    QDir nativePluginsDir = pluginsDir;
    nativePluginsDir.mkdir("native");
    if (nativePluginsDir.cd("native")) {
        loadNativePlugins(nativePluginsDir);
    }

#ifdef CUTTER_ENABLE_PYTHON_BINDINGS
    QDir pythonPluginsDir = pluginsDir;
    pythonPluginsDir.mkdir("python");
    if (pythonPluginsDir.cd("python")) {
        loadPythonPlugins(pythonPluginsDir.absolutePath());
    }
#endif

    qInfo() << "Loaded" << plugins.length() << "plugin(s).";
}


void PluginManager::destroyPlugins()
{
    for (CutterPlugin *plugin : plugins) {
        delete plugin;
    }
}

void PluginManager::loadNativePlugins(const QDir &directory)
{
    for (const QString &fileName : directory.entryList(QDir::Files)) {
        QPluginLoader pluginLoader(directory.absoluteFilePath(fileName));
        QObject *plugin = pluginLoader.instance();
        if (!plugin) {
            continue;
        }
        CutterPlugin *cutterPlugin = qobject_cast<CutterPlugin *>(plugin);
        if (!cutterPlugin) {
            continue;
        }
        cutterPlugin->setupPlugin();
        plugins.append(cutterPlugin);
    }
}

#ifdef CUTTER_ENABLE_PYTHON_BINDINGS

void PluginManager::loadPythonPlugins(const QDir &directory)
{
    Python()->addPythonPath(directory.absolutePath().toLocal8Bit().data());

    for (const QString &fileName : directory.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {
        if (fileName == "__pycache__") {
            continue;
        }
        QString moduleName;
        if (fileName.endsWith(".py")) {
            QStringList l = fileName.split(".py");
            moduleName = l[0];
        } else {
            moduleName = fileName;
        }
        CutterPlugin *cutterPlugin = loadPythonPlugin(moduleName.toLocal8Bit().constData());
        if (!cutterPlugin) {
            continue;
        }
        cutterPlugin->setupPlugin();
        plugins.append(cutterPlugin);
    }

    PythonManager::ThreadHolder threadHolder;
}

CutterPlugin *PluginManager::loadPythonPlugin(const char *moduleName)
{
    PythonManager::ThreadHolder threadHolder;

    PyObject *pluginModule = PyImport_ImportModule(moduleName);
    if (!pluginModule) {
        qWarning() << "Couldn't load module for plugin:" << QString(moduleName);
        PyErr_Print();
        return nullptr;
    }

    PyObject *createPluginFunc = PyObject_GetAttrString(pluginModule, "create_cutter_plugin");
    if (!createPluginFunc || !PyCallable_Check(createPluginFunc)) {
        qWarning() << "Plugin module does not contain create_cutter_plugin() function:" << QString(moduleName);
        if (createPluginFunc) {
            Py_DECREF(createPluginFunc);
        }
        Py_DECREF(pluginModule);
        return nullptr;
    }

    PyObject *pluginObject = PyObject_CallFunction(createPluginFunc, nullptr);
    Py_DECREF(createPluginFunc);
    Py_DECREF(pluginModule);
    if (!pluginObject) {
        qWarning() << "Plugin's create_cutter_plugin() function failed.";
        PyErr_Print();
        return nullptr;
    }

    PythonToCppFunc pythonToCpp = Shiboken::Conversions::isPythonToCppPointerConvertible(reinterpret_cast<SbkObjectType *>(SbkCutterBindingsTypes[SBK_CUTTERPLUGIN_IDX]), pluginObject);
    if (!pythonToCpp) {
        qWarning() << "Plugin's create_cutter_plugin() function did not return an instance of CutterPlugin:" << QString(moduleName);
        return nullptr;
    }
    CutterPlugin *plugin;
    pythonToCpp(pluginObject, &plugin);
    if (!plugin) {
        qWarning() << "Error during the setup of CutterPlugin:" << QString(moduleName);
        return nullptr;
    }
    return plugin;
}
#endif
