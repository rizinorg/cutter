#ifndef CUTTERSAMPLEPLUGIN_H
#define CUTTERSAMPLEPLUGIN_H

#include <CutterPlugin.h>

#include <QLabel>

class CutterSamplePlugin : public QObject, CutterPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "re.rizin.cutter.plugins.CutterPlugin")
    Q_INTERFACES(CutterPlugin)

public:
    void setupPlugin() override;
    void setupInterface(MainWindow *main) override;

    QString getName() const override { return "SamplePlugin"; }
    QString getAuthor() const override { return "xarkes"; }
    QString getDescription() const override { return "Just a sample plugin."; }
    QString getVersion() const override { return "1.0"; }
};

class CutterSamplePluginWidget : public CutterDockWidget
{
    Q_OBJECT

public:
    explicit CutterSamplePluginWidget(MainWindow *main);

private:
    QLabel *text;

private slots:
    void on_seekChanged(RVA addr);
    void on_buttonClicked();
};

#endif // CUTTERSAMPLEPLUGIN_H
