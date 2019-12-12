
#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QDir>

class CutterPlugin;

class PluginManager: public QObject
{
Q_OBJECT

public:
    static PluginManager *getInstance();

    PluginManager();
    ~PluginManager();

    /**
     * @brief Load all plugins, should be called once on application start
     */
    void loadPlugins();

    /**
     * @brief Destroy all loaded plugins, should be called once on application shutdown
     */
    void destroyPlugins();

    const QList<CutterPlugin *> &getPlugins()   { return plugins; }

    QVector<QDir> getPluginDirectories() const;
    QString getUserPluginsDirectory() const;

private:
    QList<CutterPlugin *> plugins;

    void loadNativePlugins(const QDir &directory);
    void loadPluginsFromDir(const QDir &pluginsDir, bool writable = false);

#ifdef CUTTER_ENABLE_PYTHON_BINDINGS
    void loadPythonPlugins(const QDir &directory);
    CutterPlugin *loadPythonPlugin(const char *moduleName);
#endif
};

#define Plugins() (PluginManager::getInstance())

#endif //PLUGINMANAGER_H
