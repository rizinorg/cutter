
#include <cassert>

#ifdef CUTTER_ENABLE_PYTHON_BINDINGS
#include <Python.h>
#include <cutterbindings_python.h>
#include "PythonManager.h"
#endif

#include "PluginManager.h"
#include "CutterPlugin.h"
#include "CutterConfig.h"

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

void PluginManager::loadPlugins(bool enablePlugins)
{
    assert(plugins.empty());

    if (!enablePlugins) {
        // [#2159] list but don't enable the plugins
        return;
    }

    QString userPluginDir = getUserPluginsDirectory();
    if (!userPluginDir.isEmpty()) {
        loadPluginsFromDir(QDir(userPluginDir), true);
    }
    const auto pluginDirs = getPluginDirectories();
    for (auto &dir : pluginDirs) {
        if (dir.absolutePath() == userPluginDir) {
            continue;
        }
        loadPluginsFromDir(dir);
    }
}

void PluginManager::loadPluginsFromDir(const QDir &pluginsDir, bool writable)
{
    qInfo() << "Plugins are loaded from" << pluginsDir.absolutePath();
    int loadedPlugins = plugins.size();
    if (!pluginsDir.exists()) {
        return;
    }

    QDir nativePluginsDir = pluginsDir;
    if (writable) {
        nativePluginsDir.mkdir("native");
    }
    if (nativePluginsDir.cd("native")) {
        loadNativePlugins(nativePluginsDir);
    }

#ifdef CUTTER_ENABLE_PYTHON_BINDINGS
    QDir pythonPluginsDir = pluginsDir;
    if (writable) {
        pythonPluginsDir.mkdir("python");
    }
    if (pythonPluginsDir.cd("python")) {
        loadPythonPlugins(pythonPluginsDir.absolutePath());
    }
#endif

    loadedPlugins = plugins.size() - loadedPlugins;
    qInfo() << "Loaded" << loadedPlugins << "plugin(s).";
}

void PluginManager::PluginTerminator::operator()(CutterPlugin *plugin) const
{
    plugin->terminate();
    delete plugin;
}

void PluginManager::destroyPlugins()
{
    plugins.clear();
}

QVector<QDir> PluginManager::getPluginDirectories() const
{
    QVector<QDir> result;
    QStringList locations = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation);
    for (auto &location : locations) {
        result.push_back(QDir(location).filePath("plugins"));
    }

#ifdef APPIMAGE
    {
        auto plugdir = QDir(QCoreApplication::applicationDirPath()); // appdir/bin
        plugdir.cdUp(); // appdir
        if (plugdir.cd("share/RadareOrg/Cutter/plugins")) { // appdir/share/RadareOrg/Cutter/plugins
            result.push_back(plugdir);
        }
    }
#endif

#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0) && defined(Q_OS_UNIX)
    QChar listSeparator = ':';
#else
    QChar listSeparator = QDir::listSeparator();
#endif
    QString extra_plugin_dirs = CUTTER_EXTRA_PLUGIN_DIRS;
    for (auto& path : extra_plugin_dirs.split(listSeparator, CUTTER_QT_SKIP_EMPTY_PARTS)) {
        result.push_back(QDir(path));
    }

    return result;
}


QString PluginManager::getUserPluginsDirectory() const
{
    QString location = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (location.isEmpty()) {
        return QString();
    }
    QDir pluginsDir(location);
    pluginsDir.mkpath("plugins");
    if (!pluginsDir.cd("plugins")) {
        return QString();
    }
    return pluginsDir.absolutePath();
}

void PluginManager::loadNativePlugins(const QDir &directory)
{
    for (const QString &fileName : directory.entryList(QDir::Files)) {
        QPluginLoader pluginLoader(directory.absoluteFilePath(fileName));
        QObject *plugin = pluginLoader.instance();
        if (!plugin) {
            auto errorString = pluginLoader.errorString();
            if (!errorString.isEmpty()) {
                qWarning() << "Load Error for plugin" << fileName << ":" << errorString;
            }
            continue;
        }
        PluginPtr cutterPlugin{qobject_cast<CutterPlugin *>(plugin)};
        if (!cutterPlugin) {
            continue;
        }
        cutterPlugin->setupPlugin();
        plugins.push_back(std::move(cutterPlugin));
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
            moduleName = fileName.chopped(3);
        } else {
            moduleName = fileName;
        }
        PluginPtr cutterPlugin{loadPythonPlugin(moduleName.toLocal8Bit().constData())};
        if (!cutterPlugin) {
            continue;
        }
        cutterPlugin->setupPlugin();
        plugins.push_back(std::move(cutterPlugin));
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
