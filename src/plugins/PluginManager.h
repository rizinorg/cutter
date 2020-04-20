
#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <QObject>
#include <QDir>
#include <memory>
#include <vector>

class CutterPlugin;

class PluginManager: public QObject
{
Q_OBJECT

public:
    static PluginManager *getInstance();

    class PluginTerminator
    {
    public:
        void operator()(CutterPlugin*) const;
    };
    using PluginPtr = std::unique_ptr<CutterPlugin, PluginTerminator>;

    PluginManager();
    ~PluginManager();

    /**
     * @brief Load all plugins, should be called once on application start
     * @param enablePlugins set to false if plugin code shouldn't be started
     */
    void loadPlugins(bool enablePlugins = true);

    /**
     * @brief Destroy all loaded plugins, should be called once on application shutdown
     */
    void destroyPlugins();

    const std::vector<PluginPtr> &getPlugins()   { return plugins; }

    QVector<QDir> getPluginDirectories() const;
    QString getUserPluginsDirectory() const;

private:
    std::vector<PluginPtr> plugins;

    void loadNativePlugins(const QDir &directory);
    void loadPluginsFromDir(const QDir &pluginsDir, bool writable = false);

#ifdef CUTTER_ENABLE_PYTHON_BINDINGS
    void loadPythonPlugins(const QDir &directory);
    CutterPlugin *loadPythonPlugin(const char *moduleName);
#endif
};

#define Plugins() (PluginManager::getInstance())

#endif //PLUGINMANAGER_H
