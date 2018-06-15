#include <QLabel>
#include <QHBoxLayout>

#include "CutterSamplePlugin.h"
#include "utils/TempConfig.h"

void CutterSamplePlugin::setupPlugin(CutterCore *core)
{
    this->core = core;
    this->name = "SamplePlugin";
    this->description = "Just a sample plugin.";
    this->version = "1.0";
    this->author = "xarkes";
}

CutterDockWidget* CutterSamplePlugin::setupInterface(MainWindow *main, QAction* actions)
{
    // Instantiate dock widget
    dockable = new CutterSamplePluginWidget(main, actions);
    return dockable;
}

CutterSamplePluginWidget::CutterSamplePluginWidget(MainWindow *main, QAction *action) :
    CutterDockWidget(main, action)
{
    this->setWindowTitle("Sample Plugin");
    QVBoxLayout *layout = new QVBoxLayout(this);
    this->setLayout(layout);

    text = new QLabel(this);
    text->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    layout->addWidget(text);

    connect(Core(), &CutterCore::seekChanged, this, &CutterSamplePluginWidget::on_seekChanged);
}

void CutterSamplePluginWidget::on_seekChanged(RVA addr)
{
    Q_UNUSED(addr);
    QString res;
    {
        TempConfig tempConfig;
        tempConfig.set("scr.color", 0);
        res = Core()->cmd("?E `pi 1`");
    }
    qDebug() << res;
    text->setText(res);
}
