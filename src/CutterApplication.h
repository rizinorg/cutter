#ifndef CUTTERAPPLICATION_H
#define CUTTERAPPLICATION_H

#include "core/MainWindow.h"

#include <QApplication>
#include <QEvent>
#include <QList>
#include <QProxyStyle>

enum class AutomaticAnalysisLevel : ut8 { Ask, None, AAA, AAAA };

struct CutterCommandLineOptions
{
    QStringList args;
    AutomaticAnalysisLevel analysisLevel = AutomaticAnalysisLevel::Ask;
    InitialOptions fileOpenOptions;
    QString pythonHome;
    bool outputRedirectionEnabled = true;
    bool enableCutterPlugins = true;
    bool enableRizinPlugins = true;
};

/**
 * @brief Main application class for Cutter
 */
class CutterApplication : public QApplication
{
    Q_OBJECT

public:
    CutterApplication(int &argc, char **argv);
    ~CutterApplication();

    MainWindow *getMainWindow() { return mainWindow; }

    void launchNewInstance(const QStringList &args = {});

    InitialOptions getInitialOptions() const { return clOptions.fileOpenOptions; }
    void setInitialOptions(const InitialOptions &options) { clOptions.fileOpenOptions = options; }
    QStringList getArgs() const;

protected:
    bool event(QEvent *e);

private:
    /**
     * @brief Load and translations depending on Language settings
     * @return true on success
     */
    bool loadTranslations();
    /**
     * @brief Parse commandline options and store them in a structure.
     * @return false if options have error
     */
    bool parseCommandLineOptions();

private:
    bool fileAlreadyDropped;
    CutterCore core;
    MainWindow *mainWindow;
    CutterCommandLineOptions clOptions;
};

/**
 * @brief CutterProxyStyle is used to force shortcuts displaying in context menu
 */
class CutterProxyStyle : public QProxyStyle
{
    Q_OBJECT
public:
    /**
     * @brief it is enough to get notification about QMenu polishing to force shortcut displaying
     */
    void polish(QWidget *widget) override;
};

#endif // CUTTERAPPLICATION_H
