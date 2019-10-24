#ifndef CUTTERPLUGIN_H
#define CUTTERPLUGIN_H

class CutterPlugin;
class MainWindow;

#include "core/Cutter.h"
#include "widgets/CutterDockWidget.h"

class CutterPlugin 
{
public:
    virtual ~CutterPlugin() = default;

    /**
     * @brief Initialize the Plugin
     *
     * called right when the plugin is loaded initially
     */
    virtual void setupPlugin() = 0;

    /**
     * @brief Setup any UI components for the Plugin
     * @param main the MainWindow to add any UI to
     *
     * called after Cutter's core UI has been initialized
     */
    virtual void setupInterface(MainWindow *main) = 0;

    /**
     * @brief Register any decompiler implemented by the Plugin
     *
     * called during initialization of Cutter, after setupPlugin()
     */
    virtual void registerDecompilers() {}

    /**
     * @brief Shutdown the Plugin
     *
     * called just before the Plugin is deleted.
     * This method is usually only relevant for Python Plugins where there is no
     * direct equivalent of the destructor.
     */
    virtual void terminate() {};

    virtual QString getName() const = 0;
    virtual QString getAuthor() const = 0;
    virtual QString getDescription() const = 0;
    virtual QString getVersion() const = 0;
};

#define CutterPlugin_iid "org.radare.cutter.plugins.CutterPlugin"

Q_DECLARE_INTERFACE(CutterPlugin, CutterPlugin_iid)

#endif // CUTTERPLUGIN_H
