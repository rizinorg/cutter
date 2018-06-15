#ifndef CUTTERSAMPLEPLUGIN_H
#define CUTTERSAMPLEPLUGIN_H

#include <QObject>
#include <QtPlugin>
#include "CutterPlugin.h"

class CutterSamplePlugin : public QObject, CutterPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.radare.cutter.plugins.CutterPlugin")
    Q_INTERFACES(CutterPlugin)

public:
    void setupPlugin(CutterCore *core) override;
    CutterDockWidget* setupInterface(MainWindow *main, QAction *action = nullptr) override;
};

class CutterSamplePluginWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit CutterSamplePluginWidget(MainWindow *main, QAction *action);

private:
    QLabel* text;

private slots:
    void on_seekChanged(RVA addr);
    void on_buttonClicked();
};


#endif // CUTTERSAMPLEPLUGIN_H
